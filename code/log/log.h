/**
 * @Author:         yds
 * @Date    :       2020-12-17
*/

#ifndef __LOG_H_
#define __LOG_H_


#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <sys/time.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "buffer.h"


class Log {
public:
    void init(int level, const char *path = "./log", 
                const char* suffix = ".log",
                int max_queue_capacity = 1024);
    static Log* instance();
    static void flush_log_thread();

    void write(int leve, const char* format, ...);
    void flush();

    void set_level(int level);
    int get_level();
    bool is_open();

private:
    Log();//私有化构造
    void append_log_level_title(int level);//追加日志等级
    ~Log();//私有化析构
    void ansync_write();//异步写

private:
    static const int LOG_PATH_LEN = 256;//日志路径
    static const int LOG_NAME_LEN = 256;//日志文件名长度
    static const int MAX_LINES = 50000;//每个日志最大行
    
    const char* path_;//日志文件路径
    const char* suffix_;//日志文件后缀

    int line_count_;//日志当前行数
    int today_;//标记当天，同一天的日志后缀1,2,3....

    bool is_open_;//是否打开日志系统

    Buffer buffer_;//要输出的日志内容
    int level_;//日志等级
    bool is_async_;//是否开启异步写日志

    FILE* fp_;//日志文件
    std::unique_ptr<BlockQueue<std::string>> deque_;//阻塞缓存日志消息
    std::mutex mtx_;//日志队列锁
    std::unique_ptr<std::thread> write_thread_;//写日志线程
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::instance();\
        if (log->is_open() && log->get_level() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);


#endif // !__LOG_H_