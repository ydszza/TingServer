/**

 * @Date    :       2020-12-20
*/

#ifndef __HTTPREQUEST_H_
#define __HTTPREQUEST_H_


#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <regex>
#include "../buffer/buffer.h"
#include "../log/log.h"


class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADER,
        BODY,
        FINISH
    };

    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_RESOURCE,
        FILE_REQUEST,
        INTENAL_ERROR,
        CLOSE_CONTENTION
    };

    HttpRequest();
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer& buffer);

    std::string get_path() const;
    std::string& get_path();
    std::string get_method() const;
    std::string get_version() const;
    std::string get_post(const std::string& key) const;
    std::string get_post(const char* key) const;

    bool is_keepalive() const;

private:
    bool parse_request_line(const std::string& line);
    void parse_headers(const std::string& line);
    void parse_body(const std::string& line);

    void parse_path();
    void parse_post();
    void parse_form_urlencoded();

    static int convert_hex(char ch);

private:
    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> headers_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};

#endif // !__HTTPREQUEST_H_