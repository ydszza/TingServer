/**

 * @Date    :       2020-12-20
*/
#ifndef __HTTPRESPONSE_H_
#define __HTTPRESPONSE_H_

#include <string>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "buffer.h"
#include "log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& sir_dir, const std::string& path, 
              bool is_keepalive = false, int code = -1);
    void make_response(Buffer& buffer);
    void unmap_file();
    char* get_file_mmptr();
    size_t get_file_len() const;
    void error_content(Buffer& buff, std::string message);
    int get_code() const { return code_; };

private:
    void add_state_line(Buffer& buffer);
    void add_header(Buffer& buffer);
    void add_content(Buffer& buffer);

    void error_html();
    std::string get_file_type();

private:
    int code_;

    bool is_keepalive_;

    std::string path_;
    std::string src_dir_;//请求资源路径
    
    char* mm_file_;//文件映射地址
    struct stat mm_file_stat_;//映射文件的信息

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;//后缀类型
    static const std::unordered_map<int, std::string> CODE_STATUS;//错误状态
    static const std::unordered_map<int, std::string> CODE_PATH;//错误代码页面路径
};

#endif // !__HTTPRESPONSE_H_