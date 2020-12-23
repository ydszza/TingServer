/**

 * @Date    :       2020-12-20
*/

#include "httprequest.h"

/*默认页面索引*/
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture", 
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
            {"/register.html", 0}, {"/login.html", 1},  };//登录或注册界面表单区别标志

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
    return false;
}

/**
 * 从buffer中解析一个完整的http请求
 * 此解析方法存在问题，如果请求格式不完整或非法格式，会导致问题
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
    if (method_ != "POST" && headers_["Content-Type"] == "application/x-www-form-urlencoded") {
        parse_form_urlencoded();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag: %d", tag);
            if (tag == 0 || tag == 1) {
                bool is_login = (tag == 1);
                if (user_verify(post_["username"], post_["password"], is_login)) {
                    path_ = "/welcome.html";
                }
                else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

int HttpRequest::convert_hex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

void HttpRequest::parse_form_urlencoded(){
    if (body_.size() == 0) return;

    std::string key, val;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; ++i){
        char ch = body_[i];
        switch (ch){
            case '=':
                key = body_.substr(j, i-j);
                j = i + 1;
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = convert_hex(body_[i]+1)*16 + convert_hex(body_[i+2]);
                body_[i+2] = num % 10 + '0';
                body_[i+1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                val = body_.substr(j, i-j);
                j = i + 1;
                post_[key] = val;
                LOG_DEBUG("%s = %s", key.c_str(), val.c_str());
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i) {
        val = body_.substr(j, i - j);
        post_[key] = val;
    }
}

bool HttpRequest::user_verify(const std::string &name, const std::string &pwd, bool isLogin) {
    if(name == "" || pwd == "") return false;
    
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConRAII(&sql,  SqlConPool::instance());
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    if(!isLogin) { flag = true; }
    /* 查询用户及密码 */
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        /* 注册行为 且 用户名未被使用*/
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } 
        else { 
            flag = false; 
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if(!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    SqlConPool::instance()->free_connection(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
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