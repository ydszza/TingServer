/**

 * @Date    :       2020-12-17
*/

#include "log.h"

Log::Log() {
    line_count_ = 0;
    today_ = 0;

    is_async_ = false;
    write_thread_ = nullptr;

    deque_ = nullptr;

    fp_ = nullptr;
}

Log::~Log() {
    if (write_thread_ && write_thread_->joinable()) {
        while (!deque_->empty()) {//让写日志线程吧缓存日志全部写完
            deque_->flush();
        }
        deque_->close();
        write_thread_->join();
    }
    if (fp_) {//关闭日志文件
        std::lock_guard<std::mutex> lock(mtx_);
        flush();
        fclose(fp_);
    }
}

/**
 * 日志系统初始化
 * 创建日环缓存队列
 * 创建写日志线程
 * 打开日志文件
*/
void Log::init(int level, const char* path,
                const char* suffix, int max_queue_capacity) {
    is_open_ = true;//开启日志系统
    level_ = level;//设置日志等级
    //判断是否开启异步写日志
    if (max_queue_capacity > 0) {
        is_async_ = true;//开启异步写日志
        //创建日志缓存队列
        std::unique_ptr<BlockQueue<std::string>> new_queue(new BlockQueue<std::string>());
        deque_ = std::move(new_queue);

        //创建异步写日志线程
        std::unique_ptr<std::thread> new_thread(new std::thread(flush_log_thread));
        write_thread_ = std::move(new_thread);
    }
    else {
        is_async_ = false;
    }

    line_count_ = 0;
    
    //获取当前时间
    time_t timer = time(nullptr);
    struct tm *sys_time = localtime(&timer);//将时间转化为年月日
    struct tm t = *sys_time;//此处存在内存泄漏？？？？

    path_ = path;//日志文件路径
    suffix_ = suffix;//日志文件后缀
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN-1, "%s/%04d_%02d_%02d%s",//log/年月日.log
            path_, t.tm_year, t.tm_mon, t.tm_mday, suffix_);
    today_ = t.tm_mday;//标记当天

    {
        //清空buffer，创建或打开日志文件
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.retrieve_all();

        if (fp_) {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(file_name, "a");
        if (!fp_) {
            mkdir(path_, 0777);
            fp_ = fopen(file_name, "a");
        }
        assert(fp_);
    }
}

/**
 * 单例模式，全局实例化一个日志对象
*/
Log* Log::instance() {
    static Log inst;
    return &inst;
}

/**
 * 异步写日志到日志文件
 * 线程任务函数
 * 这样写是为了在线程能够调用类函数
*/
void Log::flush_log_thread() {
    Log::instance()->ansync_write();
}

/**
 * 异步写日志线程任务函数
*/
void Log::ansync_write() {
    std::string str = "";
    while (deque_->pop(str)) {
        std::lock_guard<std::mutex> lock(mtx_);
        fputs(str.c_str(), fp_);
    }
}

/**
 * 往日志系统写入日志信息
 * 日志格式：年月日 时间 日志等级 具体日志信息
*/
void Log::write(int level, const char* format, ...) {
    //获取当前时间
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    //转化为年月日
    struct tm* sys_time = localtime(&now.tv_sec);
    struct tm t = *sys_time;

    //如果一个日志文件写满或过了一天则创建新的文件
    if (today_ != t.tm_mday || (line_count_ && (line_count_ % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> lock(mtx_);//何必在此处拿锁？？？？？
        lock.unlock();

        char new_file_name[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year+1900, t.tm_mon+1, t.tm_mday);

        if (today_ != t.tm_mday) {
            snprintf(new_file_name, LOG_NAME_LEN-72, "%s/%s%s", path_, tail, suffix_);
            today_ = t.tm_mday;
            line_count_ = 0;
        }
        else {
            snprintf(new_file_name, LOG_NAME_LEN-72, "%s/%s-%d%s", 
                    path_, tail, (line_count_/MAX_LINES), suffix_);
        }

        //把旧日志写到已满文件然后关闭旧的日志文件
        //然后打开新的日志文件
        lock.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(new_file_name, "a");
        assert(fp_);
    }

    //正常情况直接写入日志消息
    {
        std::lock_guard<std::mutex> lock(mtx_);
        line_count_++;

        //写入此条日志时间
        int n = snprintf(buffer_.get_begin_write_ptr(), 128, "%d-%02d-%02d %02d:%02d:%02d.%6ld",
                        t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buffer_.has_written(n);
        append_log_level_title(level);//添加日志等级

        //解析可变参数
        va_list args;
        va_start(args, format);
        //把日志内容添加
        int m = vsnprintf(buffer_.get_begin_write_ptr(), buffer_.get_readable_bytes(), format, args);
        va_end(args);
        buffer_.has_written(m);

        buffer_.append("\n\0", 2);

        //写入日志
        if (is_async_) {
            deque_->push_back(buffer_.retrieve_all_tostr());
        }
        else {
            fputs(buffer_.peek(), fp_);//此处是否有bug？？？？
        }
        buffer_.retrieve_all();
    }
}

/**
 * 通知线程写日志到文件
*/
void Log::flush() {
    if (is_async_) deque_->flush();
    fflush(fp_);
}

/**
 * 设置日志等级
*/
void Log::set_level(int level) {
    std::lock_guard<std::mutex> lock(mtx_);
    level_ = level;
}

/**
 * 获取日志等级
*/
int Log::get_level() {
    std::lock_guard<std::mutex> lock(mtx_);
    return level_;
}

/**
 * 获取是否开启日志系统
*/
bool Log::is_open() {
    return is_open_;
}

/**
 * 添加日志等级
*/
void Log::append_log_level_title(int level) {
    switch(level) {
    case 0:
        buffer_.append("[debug]: ", 9);
        break;
    case 1:
        buffer_.append("[info] : ", 9);
        break;
    case 2:
        buffer_.append("[warn] : ", 9);
        break;
    case 3:
        buffer_.append("[error]: ", 9);
        break;
    default:
        buffer_.append("[info] : ", 9);
        break;
    }
}