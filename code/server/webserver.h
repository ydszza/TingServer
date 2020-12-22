/**
 * @Date    :       2020-12-20
*/

#ifndef __WEBSERVER_H_
#define __WEBSERVER_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>
#include <functional>
#include <unordered_map>
#include "../timer/heaptimer.h"
#include "../pool/threadpool.h"
#include "../event/epoller.h"
#include "../http/httpconn.h"


class WebServer {
public:
    WebServer(int port, int trig_mode, int timeout_ms, bool opt_linger, 
              int thread_num, bool open_log, int log_level, int log_queue_size);
    ~WebServer();
    void start();

private:    
    bool init_socket();
    void init_event_mode(int trig_mode);
    void add_client(int fd, sockaddr_in addr);

    void deal_listen();
    void deal_write(HttpConn* client);
    void deal_read(HttpConn* client);

    void send_error(int fd, const char* info);
    void extent_time(HttpConn* client);
    void close_connection(HttpConn* client);

    void on_read(HttpConn* client);
    void on_write(HttpConn* client);
    void on_process(HttpConn* client);

    static int set_fd_nonblock(int fd);

private:
    static const int MAX_FD = 65536;

    int port_;
    bool opt_linger_;//优雅关闭
    int timeout_ms_;
    bool is_close_;
    int listen_fd_;
    char* src_dir_;

    uint32_t listen_event_;
    uint32_t conn_event_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;

};


#endif // !__WEBSERVER_H_