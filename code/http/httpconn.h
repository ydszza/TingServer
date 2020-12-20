/**

 * @Date    :       2020-12-20
*/

#ifndef __HTTPCON_H_
#define __HTTPCON_H_


#include <arpa/inet.h>
#include <atomic>
#include <sys/uio.h>
#include "buffer.h"
#include "log.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void init(int fd, const struct sockaddr_in& addr);

    ssize_t read(int* error);
    ssize_t write(int* error);
    void close_conn();

    int get_fd() const;
    int get_port() const;
    const char* get_ip() const;
    sockaddr_in get_addr() const;

    bool process();
    int to_write_bytes();//还要发送的数据量大小

    bool is_keepalive() const;

    static bool is_ET;
    static const char* src_dir;
    static std::atomic<int> user_count;

private:
    int fd_;
    struct sockaddr_in addr_;

    bool is_close_;

    struct iovec iov_[2];
    int iov_len;

    Buffer read_buffer_;
    Buffer write_buffer_;

    HttpRequest request_;
    HttpResponse response_;
};

#endif // !__HTTPCON_H_