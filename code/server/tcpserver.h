/**
 * @Author:         yds
 * @Date    :       2020-12-18
*/

#ifndef __SERVER_H_
#define __SERVER_H_


#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>
#include <unordered_map>
#include "heaptimer.h"
#include "threadpool.h"
#include "epoller.h"


class TcpServer {
public:
    TcpServer(int port, int trig_mode, int timeout_ms, bool opt_linger, 
           int conpool_num, int thread_num,
           bool open_log, int log_level, int log_queue_size);
    ~TcpServer();
    void start();

private:    
    bool init_socket();
    void init_event_mode(int trig_mode);
    void add_client(int fd, sockaddr_in addr);

    void deal_listen();
    void deal_write();
    void deal_read();

    void send_error(int fd, const char* info);
    void extent_time();
    void close_connection();

    void on_read();
    void on_write();
    void on_process();

    static const int MAX_FD = 65536;

    static int set_fd_nonblock(int fd);

private:
    int port_;
    bool opt_linger_;//epoll模式
    int timeout_ms_;
    bool is_close_;
    int listen_fd_;
    char* src_dir_;

    uint32_t listen_event_;
    uint32_t conn_event_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpCon> users_;

};

#endif // !__SERVER_H_