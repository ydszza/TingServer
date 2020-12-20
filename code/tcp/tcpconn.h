/**

 * @Date    :       2020-12-19
*/
#ifndef __TCPCONN_H_
#define __TCPCONN_H_

#include <arpa/inet.h>
#include <atomic>
#include <sys/uio.h>
#include "buffer.h"
#include "log.h"


class TcpConn {
public:
    TcpConn();
    ~TcpConn();

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
    static std::atomic<int> user_count;

private:
    bool parse();

private:
    int fd_;
    struct sockaddr_in addr_;

    bool is_close_;

    Buffer read_buffer_;
    Buffer write_buffer_;
};

#endif // !__TCPCONN_H_