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
    LOG_INFO("client[%d](%s:%d) in, user_count: %d",fd_, get_ip(), get_port(), static_cast<int>(user_count));
}

void TcpConn::close_conn() {
    if (!is_close_) {
        is_close_ = true;
        user_count--;
        close(fd_);
        LOG_INFO("client[%d](%s:%d) quit, user_count: %d",fd_, get_ip(), get_port(), static_cast<int>(user_count));
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
    return write_buffer_.get_readable_bytes();
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
        len = write_buffer_.write_to_fd(fd_, error);
        if (len <= 0) {
            *error = errno;
            break;
        }
        if (write_buffer_.get_readable_bytes() == 0) {//数据全部发送完毕
            break;
        }
        else {
            write_buffer_.retrieve(len);
        }
    } while (is_ET || to_write_bytes());
    return len;
}

bool TcpConn::process() {
    if (read_buffer_.get_readable_bytes() <= 0) return false;
    else if (parse()) {
        
    }
    return false;
}

bool TcpConn::parse() {
    //解析读去到的数据，如果包含一个完整的命令则执行然后返回结果
    if (read_buffer_.get_readable_bytes() <= 0) return false;

    //解析一条命令
    const char CTRL[] = "00";
    const char* cmd_end = std::search(read_buffer_.peek(), read_buffer_.get_begin_write_ptr_const(), CTRL, CTRL+2);
    //if (cmd_end == read_buffer_.get_begin_write_ptr_const()) return false;

    //执行命令
    std::string cmd(read_buffer_.peek(), cmd_end);
    if (cmd == "ls") {
        char *result = run_cmd("ls");//执行ls
        write_buffer_.append(result, strlen(result));
        return true;
    }

    return false;
}


char* TcpConn::run_cmd(const char *cmd) {
    char *data = static_cast<char *>(malloc(16384));
    bzero(data, 16384);
    FILE *fdp;
    const int max_buffer = 256;
    char buffer[max_buffer];
    fdp = popen(cmd, "r");
    char *data_index = data;
    if (fdp) {
        while (!feof(fdp)) {
            if (fgets(buffer, max_buffer, fdp) != NULL) {
                int len = strlen(buffer);
                memcpy(data_index, buffer, len);
                data_index += len;
            }
        }
        pclose(fdp);
    }
    return data;
}