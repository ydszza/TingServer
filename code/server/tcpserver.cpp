/**

 * @Date    :       2020-12-20
*/

#include "tcpserver.h"

TcpServer::TcpServer(int port, int trig_mode, int timeout_ms, bool opt_linger,
                     int conpool_num, int thread_num,
                     bool open_log, int log_level, int log_queue_size) : 
                     port_(port), opt_linger_(opt_linger), timeout_ms_(timeout_ms), is_close_(false),
                     timer_(new HeapTimer()), threadpool_(new ThreadPool(thread_num)),epoller_(new Epoller())
{
    //设置端口监听和读写事件的触发模式
    init_event_mode(trig_mode);

    //初始化本地端口监听
    if (!init_socket()) is_close_ = true;

    //是否开启日志系统
    if(open_log) {
        Log::instance()->init(log_level, "./log", ".log", log_queue_size);  
    }

    //打印启动server日志信息
    if (is_close_) {
        LOG_ERROR("========== Server init error!==========");
    }
    else {
        LOG_INFO("========== Server init ==========");
        LOG_INFO("Port:%d, opt_linger: %s", port_, opt_linger? "true":"false");
        LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                        (listen_event_ & EPOLLET ? "ET": "LT"),
                        (conn_event_ & EPOLLET ? "ET": "LT"));
        LOG_INFO("LogSys level: %d", log_level);
        LOG_INFO("ThreadPool num: %d",thread_num);
    } 
}

TcpServer::~TcpServer() {
    close(listen_fd_);
    is_close_ = true;
}


/**
 * 设置监听连接事件和读写事件的触发模式
*/
void TcpServer::init_event_mode(int trig_mode) {
    listen_event_ = EPOLLRDHUP;//TCP连接对端关闭
    conn_event_ = EPOLLONESHOT | EPOLLRDHUP;//一个连接仅被一个线程处理以及对端关闭连接

    //设置epoll模式，不设置则默认LT
    switch( trig_mode ) {
        case 0:
            break;//LT+LT
        case 1:
            conn_event_ |= EPOLLET;//LT+ET
            break;
        case 2:
            listen_event_ |= EPOLLET;//ET+LT
            break;
        case 3:
            listen_event_ |= EPOLLET;//ET+ET
            conn_event_ |= EPOLLET;
            break;
        default:
            listen_event_ |= EPOLLET;//ET+ET
            conn_event_ |= EPOLLET;
            break;
    }
}

/**
 * 事件循环核心
*/
void TcpServer::start() {
    int timeout = -1;
    if (!is_close_) LOG_INFO("========== Server start ==========");

    while (!is_close_) {

        //初始定时值-1，后续为定时器中时间最短的定时器
        if (timeout_ms_ > 0) timeout = timer_->get_next_tick();
        int event_cnt = epoller_->wait(timeout);

        //处理触发的事件
        for (int i = 0; i < event_cnt; ++i) {
            int fd = epoller_->get_event_fd(i);
            uint32_t events = epoller_->get_events(i);

            if (fd == listen_fd_) {//连接事件
                deal_listen();
            }
            else if (events & EPOLLIN) {//可读事件
                deal_read(&users_[fd]);
            }
            else if (events & EPOLLOUT) {//可写事件
                deal_write(&users_[fd]);
            }
            else {//出错
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

/**
 * 添加新的连接
*/
void TcpServer::add_client(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);

    if (timeout_ms_ > 0) {
        //添加定时事件
        timer_->add(fd, timeout_ms_, std::bind(&TcpServer::close_connection, this, &users_[fd]));
    }

    epoller_->add_fd(fd, conn_event_|EPOLLIN);

    set_fd_nonblock(fd);

    LOG_INFO("client[fd] in", users_[fd].get_fd());
}


/**
 * 给连接发送忙消息然后断开连接
*/
void TcpServer::send_error(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

/**
 * 断开连接
*/
void TcpServer::close_connection(TcpConn* client) {
    LOG_INFO("CLIENT[%d] quit", client->get_fd());
    epoller_->del_fd(client->get_fd());
    client->close_conn();
}


/**
 * 处理新的连接事件
*/
void TcpServer::deal_listen() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    do {
        int fd = accept(listen_fd_, (struct sockaddr *)&addr, &addrlen);
        if (fd < 0) return;
        else if (TcpConn::user_count >= MAX_FD) {
            send_error(fd, "server busy!");
            LOG_WARN("server busy, client is full!");
            close(fd);//连接过多关闭，增加关闭连接
            return;
        }
        add_client(fd, addr);
    } while (conn_event_ & EPOLLET);
    //ET模式读到空为止,因为多个连接事件一起到达仅触发一次
}

/**
 * 处理读事件
*/
void TcpServer::deal_read(TcpConn* client) {
    //将读事件回调添加到线程池事件队列
    extent_time(client);//更新此连接的定时时间
    //add task to read
    threadpool_->add_task(std::bind(&TcpServer::on_read, this, client));
}

/**
 * 处理写事件
*/
void TcpServer::deal_write(TcpConn* client) {
    extent_time(client);
    //add task to write
    threadpool_->add_task(std::bind(&TcpServer::on_write, this, client));
}

/**
 * 读回调函数
*/
void TcpServer::on_read(TcpConn* client) {
    int ret = -1;
    int read_error = 0;
    ret = client->read(&read_error);
    if (ret <= 0 && read_error != EAGAIN) {
        close_connection(client);
        return;
    }
    on_process(client);//处理消息,如果有一个完整的请求则注册写监听响应请求
}

/**
 * 写回调函数
*/
void TcpServer::on_write(TcpConn* client) {
    int ret = -1;
    int write_error = 0;
    ret = client->write(&write_error);
    if (client->to_write_bytes()) {
        if (client->is_keepalive()) {
            on_process(client);
            return;
        }
    }
    else if (ret < 0) {
        if (write_error == EAGAIN) {
            epoller_->mod_fd(client->get_fd(), conn_event_|EPOLLOUT);
            return;
        }
    }
    close_connection(client);
}

/**
 * 处理数据看看是否有一个完整的消息
*/
void TcpServer::on_process(TcpConn* client) {
    if (client->process()) {
        epoller_->mod_fd(client->get_fd(), conn_event_|EPOLLOUT);//设置写监听
    }
    else {
        epoller_->mod_fd(client->get_fd(), conn_event_|EPOLLIN);
    }
}

/**
 * 更新连接的定时器
*/
void TcpServer::extent_time(TcpConn* client) {
    if (timeout_ms_ > 0) timer_->adjust(client->get_fd(), timeout_ms_);
}

/**
 * 初始化监听端口
*/
bool TcpServer::init_socket() {
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 0) {
        LOG_ERROR("port %d error", port_);
        return false;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //close()不会立刻返回，内核会延迟一段时间，这个时间就由l_linger的值来决定。
    //如果超时时间到达之前，发送完未发送的数据(包括FIN包)并得到另一端的确认，
    //close()会返回正确，socket描述符优雅性退出。
    struct linger opt_linger = {0};
    if (opt_linger_) {
        //优雅关闭连接：直到所剩数据全部去发送完毕
        opt_linger.l_linger = 1;
        opt_linger.l_onoff = 1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        LOG_ERROR("creat socket erorr!");
        return false;
    }

    //设置优雅关闭
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger, sizeof(opt_linger));
    if (ret < 0) {
        close(listen_fd_);
        LOG_ERROR("init linger error!");
        return false;
    }

    //设置端口复用
    int optval = 1;
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval));
    if (ret < 0) {
        close(listen_fd_);
        LOG_ERROR("set socket setopt error!");
        return false;
    }

    //绑定
    ret = bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        close(listen_fd_);
        LOG_ERROR("bind error!");
        return false;
    }

    //添加到epoll监听
    if (!epoller_->add_fd(listen_fd_, listen_event_|EPOLLIN)) {
        close(listen_fd_);
        LOG_ERROR("add listen fd error!");
        return false;
    }

    //监听端口
    ret = listen(listen_fd_, 6);
    if (ret < 0) {
        close(listen_fd_);
        LOG_ERROR("listEN error!");
        return false;
    }

    //设置非阻塞
    set_fd_nonblock(listen_fd_);
    LOG_INFO("server port %d", port_);

    return true;
}

/**
 * 设置fd非阻塞
*/
int TcpServer::set_fd_nonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)|O_NONBLOCK);
}