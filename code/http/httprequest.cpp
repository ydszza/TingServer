/**

 * @Date    :       2020-12-20
*/

#include "httprequest.h"

/*默认页面索引*/
const std::unordered_set<std::string> DEFAULT_HTML = {
    "/index", "/picture"
};

const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;//登录或注册界面表单区别标志

HttpRequest::HttpRequest() {
    init();
}

/**
 * 初始化request，因为这个类会被重复利用
*/
void HttpRequest::init() {
    state_ = REQUEST_LINE;
    method_ = path_ = version_ = body_ = "";
    headers_.clear();
    post_.clear();
}

bool HttpRequest::is_keepalive() const {
    if (headers_.count("Connection"))
        return headers_.find("Connection")->second == "keep-alive" && version_ == "1.1";
}

/**
 * 从buffer中解析一个完整的http请求
*/
bool HttpRequest::parse(Buffer& buffer) {
    const char CTRL[] = "\r\n";
    if (buffer.get_readable_bytes() <= 0) return false;

    while (buffer.get_readable_bytes() && state_ != FINISH) {

        const char* line_end = std::search(buffer.peek(), buffer.get_begin_write_ptr_const(), CTRL, CTRL+2);
        std::string line(buffer.peek(), line_end);

        switch(state_) {
            case REQUEST_LINE:
                if (!parse_request_line(line)) {
                    return false;
                }
                parse_path();
                break;
            case HEADER:
                parse_headers(line);
                if (buffer.get_readable_bytes() <= 2) state_ = FINISH;//说明没有请求体或者请求不完整
                break;
            case BODY:
                parse_body(line);
                break;
            default:
                break; 
        }
        
        if (line_end == buffer.get_begin_write_ptr()) break;
        buffer.retrieve_until(line_end+2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::parse_path() {
    if (path_ == "/") path_ = "/index.html";
    else {
        for (auto& item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                return;
            }
        }
    }
}

bool HttpRequest::parse_request_line(const std::string& line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch sub_match;

    if (regex_match(line, sub_match, patten)) {
        method_ = sub_match[1];
        path_ = sub_match[2];
        version_ = sub_match[3];
        state_ = HEADER;
        return true;
    }

    LOG_ERROR("requestline error");
    return false;
}

void HttpRequest::parse_headers(const std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch sub_match;
    if (regex_match(line, sub_match, patten)) {
        headers_[sub_match[1]] = sub_match[2];
    }
    else {
        state_ = BODY;
    }
}

void HttpRequest::parse_body(const std::string& line) {
    body_ = line;
    parse_post();
    state_ = FINISH;
    LOG_DEBUG("body: %s, len: %d", body_.c_str(), body_.length());
}

void HttpRequest::parse_post() {
    if (method_ != "POST" && headers_["Content-Type"] == "application/x-www-form-urlencoded") return ;
    //...
}

std::string HttpRequest::get_path() const {
    return path_;
}

std::string& HttpRequest::get_path(){
    return path_;
}

std::string HttpRequest::get_method() const {
    return method_;
}

std::string HttpRequest::get_version() const {
    return version_;
}

std::string HttpRequest::get_post(const std::string& key) const {
    return post_.find(key)->second;
}

std::string HttpRequest::get_post(const char* key) const {
    assert(key);
    if (post_.count(key)) return post_.find(key)->second;
    return "";
}