/**

 * @Date    :       2020-12-20
*/
#include "httpconn.h"

bool HttpConn::is_ET;
const char* HttpConn::src_dir;
std::atomic<int> HttpConn::user_count;

HttpConn::HttpConn() : fd_(-1), addr_({0}), is_close_(true) {

}

HttpConn::~HttpConn() {
    close_conn();
}

void HttpConn::init(int fd, const struct sockaddr_in& addr) {
    assert(fd > 0);
    user_count++; 
    fd_ = fd;
    addr_ = addr;
    write_buffer_.retrieve_all();
    read_buffer_.retrieve_all();
    is_close_ = false;
    LOG_INFO("client[%d](%s:%d) in, user_count: %d",fd_, get_ip(), get_port(), static_cast<int>(user_count));
}

/**
 * 取消文件映射
 * 关闭套接字
*/
void HttpConn::close_conn() {
    response_.unmap_file();
    if (!is_close_) {
        is_close_ = true;
        user_count--;
        close(fd_);
        LOG_INFO("client[%d](%s:%d) quit, user_count: %d",fd_, get_ip(), get_port(), static_cast<int>(user_count));
    }
}

int HttpConn::get_fd() const {
    return fd_;
}

int HttpConn::get_port() const {
    return addr_.sin_port;
}

const char* HttpConn::get_ip() const {
    return inet_ntoa(addr_.sin_addr);
}

sockaddr_in HttpConn::get_addr() const {
    return addr_;
}

int HttpConn::to_write_bytes() {
    return iov_[0].iov_len + iov_[1].iov_len; 
}

bool HttpConn::is_keepalive() const {
    return request_.is_keepalive();
}

ssize_t HttpConn::read(int* error) {
    ssize_t len = -1;
    do {
        len = read_buffer_.read_from_fd(fd_, error);
        if (len <= 0) break;
    } while (is_ET);

    return len;
}

ssize_t HttpConn::write(int* error) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_len);
        if (len <= 0) {//发送出错退出发送
            *error = errno;
            break;
        }
        if (iov_[0].iov_len+iov_[1].iov_len == 0) {//数据全部发送完毕
            break;
        }
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            //移动到未发送的起始位置
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            
            if (iov_[0].iov_len) {//清空已发送的数据
                write_buffer_.retrieve_all();
                iov_[1].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[1].iov_len -= len;
            write_buffer_.retrieve(len);
        }
    } while (is_ET || to_write_bytes() > 10240);//直到数据少于10k
    return len;
}

bool HttpConn::process() {
    if (read_buffer_.get_readable_bytes() <= 0) return false;
    else if (request_.parse(read_buffer_)) {
        response_.init(src_dir, request_.get_path(), request_.is_keepalive(), 200);
    }
    else {
        response_.init(src_dir, request_.get_path(), false, 400);
    }

    response_.make_response(write_buffer_);

    iov_[0].iov_base = const_cast<char *>(write_buffer_.peek());
    iov_[0].iov_len = write_buffer_.get_readable_bytes();
    iov_len = 1;

    if (response_.get_file_len() > 0 && response_.get_file_mmptr()) {
        iov_[1].iov_base = response_.get_file_mmptr();
        iov_[1].iov_len = response_.get_file_len();
        iov_len = 2;
    }

    LOG_DEBUG("file size: %d,%d to %d", response_.get_file_len(), iov_len, to_write_bytes());
    return true;
}
