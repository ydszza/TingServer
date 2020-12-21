/**

 * @Date    :       2020-12-20
*/

#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = src_dir_ = "";
    is_keepalive_ = false;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

HttpResponse::~HttpResponse() {
    unmap_file();
}

/**
 * 响应初始化
*/
void HttpResponse::init(const std::string& src_dir, const std::string& path, bool is_keepalive, int code) {
    assert(src_dir != "");
    if (mm_file_) unmap_file();
    code_ = code;
    is_keepalive_ = is_keepalive;
    path_ = path;
    src_dir_ = src_dir;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

/**
 * 生成响应
*/
void HttpResponse::make_response(Buffer& buffer) {
    if (stat((src_dir_ + path_).data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode)) {
        code_ = 404;
    }
    else if (!(mm_file_stat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    
    error_html();
    add_state_line(buffer);
    add_header(buffer);
    add_content(buffer);
}

/**
 * 获取文件映射内存地址
*/
char* HttpResponse::get_file_mmptr() {
    return mm_file_;
}

/**
 * 获取文件大小
*/
size_t HttpResponse::get_file_len() const {
    return mm_file_stat_.st_size;
}

/**
 * 设置错误页面
*/
void HttpResponse::error_html() {
    if (CODE_PATH.count(code_)) {
        path_ = CODE_PATH.find(code_)->second;
        stat((src_dir_ + path_).data(), &mm_file_stat_);
    }
}

/**
 * 添加响应状态
*/
void HttpResponse::add_state_line(Buffer& buffer) {
    std::string status;
    if (CODE_PATH.count(code_)) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(code_)->second;
    }
    buffer.append("HTTP/1.1" + std::to_string(code_) + " " + status + "\r\n");
}

/**
 * 添加响应头
*/
void HttpResponse::add_header(Buffer& buffer) {
    buffer.append("Connection: ");
    if (is_keepalive_) {
        buffer.append("keep-alive\r\n");
        buffer.append("keep-alive: max=6, timeout=120\r\n");
    } 
    else {
        buffer.append("close\r\n");
    }
    buffer.append("Content-type: " + get_file_type() + "\r\n");
}

/**
 * 添加响应body
*/
void HttpResponse::add_content(Buffer& buffer) {
    int src_fd = open((src_dir_+ path_).data(), O_RDONLY);
    if (src_fd < 0) {
        error_content(buffer, "file notfount!");
        return;
    }

    LOG_DEBUG("file path: %s", ((src_dir_ + path_).data()));
    int *mm_ret = (int *)mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (mm_ret < 0) {
        error_content(buffer, "file notfount!");
        return ;
    }
    mm_file_ = (char *)mm_ret;
    close(src_fd);
    buffer.append("Content-length: " + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

/**
 * 取消文件内存映射
*/
void HttpResponse::unmap_file() {
    if (mm_file_) {
        munmap(mm_file_, mm_file_stat_.st_size);
        mm_file_ = nullptr;
    }
}

/**
 * 获取响应文件类型头
*/
std::string HttpResponse::get_file_type() {
    std::string::size_type pos = path_.find_last_of('.');
    if (pos == std::string::npos) return "text/plain";

    std::string suffix = path_.substr(pos);
    if (SUFFIX_TYPE.count(suffix)) return SUFFIX_TYPE.find(suffix)->second;

    return "text/plain";
}

/**
 * 生成出错页面
*/
void HttpResponse::error_content(Buffer& buffer, std::string message) {
    std::string body;
    std::string status;

    body +="<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";

    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }

    body += std::to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyServer</em></body></html>";

    buffer.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buffer.append(body);
}