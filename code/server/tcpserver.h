/**

 * @Date    :       2020-12-20
*/

#ifndef __SERVER_H_
#define __SERVER_H_


#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>
#include <unordered_map>
#include "../timer/heaptimer.h"
#include "../pool/threadpool.h"
#include "../event/epoller.h"
#include "../tcp/tcpconn.h"

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
    void deal_write(TcpConn* client);
    void deal_read(TcpConn* client);

    void send_error(int fd, const char* info);
    void extent_time(TcpConn* client);
    void close_connection(TcpConn* client);

    void on_read(TcpConn* client);
    void on_write(TcpConn* client);
    void on_process(TcpConn* client);

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
    std::unordered_map<int, TcpConn> users_;

};

#endif // !__SERVER_H_