/**

 * @Date    :       2020-12-19
*/
#include "tcpconn.h"

bool TcpConn::is_ET;
std::atomic<int> TcpConn::user_count;

TcpConn::TcpConn() : fd_(-1), addr_({0}), is_close_(true) {

}

TcpConn::~TcpConn() {
    close_conn();
}

void TcpConn::init(int fd, const struct sockaddr_in& addr) {
    assert(fd > 0);
    user_count++; 
    fd_ = fd;
    addr_ = addr;
    write_buffer_.retrieve_all();
    read_buffer_.retrieve_all();
    is_close_ = false;
    LOG_INFO("client[%d](%s:%d) in, user_count: %d",fd_, get_ip(), get_port(), user_count);
}

void TcpConn::close_conn() {
    if (!is_close_) {
        is_close_ = true;
        user_count--;
        close(fd_);
        LOG_INFO("client[%d](%s:%d) quit, user_count: %d",fd_, get_ip(), get_port(), user_count);
    }
}

int TcpConn::get_fd() const {
    return fd_;
}

int TcpConn::get_port() const {
    return addr_.sin_port;
}

const char* TcpConn::get_ip() const {
    return inet_ntoa(addr_.sin_addr);
}

sockaddr_in TcpConn::get_addr() const {
    return addr_;
}

int TcpConn::to_write_bytes() {
    return iov_[0].iov_len + iov_[1].iov_len; 
}

bool TcpConn::is_keepalive() const {
    return true;
}

ssize_t TcpConn::read(int* error) {
    ssize_t len = -1;
    do {
        len = read_buffer_.read_from_fd(fd_, error);
        if (len < 0) break;
    } while (is_ET);

    return len;
}

ssize_t TcpConn::write(int* error) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_len);
        if (len <= 0) {
            *error = errno;
            break;
        }
        if (iov_[0].iov_len+iov_[1].iov_len == 0) {//数据全部发送完毕
            break;
        }
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                write_buffer_.retrieve_all();
                iov_[1].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[1].iov_len -= len;
            write_buffer_.retrieve(len);
        }
    } while (is_ET || to_write_bytes());
    return len;
}

bool TcpConn::process() {
    if (read_buffer_.get_readable_bytes() <= 0) return false;
    else if (//解析数据是否有一个完整的消息) {
        
    }
}
