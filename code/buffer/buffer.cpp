/**

 * @Date    :       2020-12-17
*/

#include "buffer.h"

/**
 * 初始化底层数据结构默认大小以及读写位置
 */
Buffer::Buffer(int default_buffer_size) : 
        buffer_(default_buffer_size), read_pos_(0), write_pos_(0) {

}

/**
 * 获取可写空间大小
*/
size_t Buffer::get_writable_bytes() const {
    return buffer_.size() - write_pos_;
}

/**
 * 获取可读空间大小
*/
size_t Buffer::get_readable_bytes() const {
    return write_pos_ - read_pos_;
}

/**
 * 获取预留空间大小
*/
size_t Buffer::get_prependable_bytes() const {
    return read_pos_;
}

/**
 * 获取可读buffer首地址
*/
const char* Buffer::peek() const {
     return get_begin_ptr() + read_pos_;
}

/**
 * 确保可写空间大小足够大
*/
void Buffer::ensure_writeable(size_t len) {
    if (get_writable_bytes() < len) {
        make_space(len);
    }
    assert(get_writable_bytes() >= len);
}

/**
 * 写入成功后更新可写位置
*/
void Buffer::has_written(size_t len) {
    write_pos_ += len;
}

/**
 * 删除len长度的可读buffer
*/
void Buffer::retrieve(size_t len) {
    assert(len <= get_readable_bytes());
    read_pos_ += len;
}

/**
 * 删除指定到指定位置
*/
void Buffer::retrieve_until(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

/**
 * 清除所有可读缓存
*/
void Buffer::retrieve_all() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = write_pos_ = 0;
}

/**
 * 清除所有可读缓存且返回可读的缓存内容
*/
std::string Buffer::retrieve_all_tostr() {
    std::string str(peek(), get_readable_bytes());
    retrieve_all();
    return str;
}

/**
 * 返回可写buffer的首地址
*/
const char* Buffer::get_begin_write_ptr_const() const {
    return get_begin_ptr() + write_pos_;
}

char* Buffer::get_begin_write_ptr() {
    return get_begin_ptr() + write_pos_;
}

/**
 * 往buffet末尾添加内容
*/
void Buffer::append(const char* str, size_t len) {
    assert(str);
    ensure_writeable(len);
    std::copy(str, str+len, get_begin_write_ptr());
    has_written(len);
}

void Buffer::append(void* data, size_t len) {
    assert(data);
    append(static_cast<char*>(data), len);
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

void Buffer::append(const Buffer& buff) {
    append(buff.peek(), buff.get_readable_bytes());
}

/**
 * 从指定的io读取数据
*/
ssize_t Buffer::read_from_fd(int fd, int* error) {
    assert(fd >= 0 && error);
    char buff[65536];
    struct iovec iov[2];
    const size_t writable = get_writable_bytes();

    //分散读，保证读完
    //先读入到buffer_, 然后读到buff中
    iov[0].iov_base = get_begin_write_ptr();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {//读出错
        *error = errno;
    }
    else if (static_cast<size_t>(len) <= writable) {//读取的内容长度小于可写缓存长度
        write_pos_ += len;
    }
    else {//读取的内容超过已有缓存长度
        write_pos_ = buffer_.size();
        append(buff, len-writable);
    }

    return len;
}

/**
 * 向指定io写入数据
*/
ssize_t Buffer::write_to_fd(int fd, int* error) {
    ssize_t len = write(fd, peek(), get_readable_bytes());
    if (len < 0) {
        *error = errno;//写出错
        return len;
    }
    read_pos_ += len;
    return len;
}

char* Buffer::get_begin_ptr() {
    return &*buffer_.begin();
}

const char* Buffer::get_begin_ptr() const {
    return &*buffer_.begin();
}

/**
 * 缓存扩容
 * 此处不同，
*/
void Buffer::make_space(size_t len) {
    if (get_writable_bytes() + get_prependable_bytes() < len) {
        buffer_.resize(write_pos_ + len + 1);
    }
    else {
        std::copy(get_begin_ptr()+read_pos_, get_begin_ptr()+write_pos_, get_begin_ptr());
        write_pos_ -= read_pos_;
        read_pos_ = 0;
    }
}