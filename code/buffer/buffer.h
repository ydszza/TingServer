/**
 * @Author:         yds
 * @Date    :       2020-12-17
*/

#ifndef __BUFFER_H_
#define __BUFFER_H_


#include <vector>
#include <atomic>
#include <string>
#include <cstring>
#include <assert.h>
#include <sys/uio.h>
#include <unistd.h>


class Buffer {
public:
    Buffer(int default_buffer_size = 1024);
    ~Buffer() = default;

    size_t get_writable_bytes() const;          //获取可写空间大小
    size_t get_readable_bytes() const;          //获取可读空间大小
    size_t get_prependable_bytes() const;       //获取预留空间大小

    const char* peek() const;                   //获取可读缓存的首地址
    void ensure_writeable(size_t len);          //确保空间足够写
    void has_written(size_t len);               //更新写入后的可写起始index

    //删除指定长度的可读buffer，从可读起始位置开始
    void retrieve(size_t len); 
    void retrieve_until(const char* end);
    void retrieve_all();
    std::string retrieve_all_tostr();

    //返回可写buffer的首地址
    const char* get_begin_write_ptr_const() const;
    char* get_begin_write_ptr();

    //往buffer尾部添加内容
    void append(const char* str, size_t len);
    void append(void* data, size_t len);
    void append(const std::string& str);
    void append(const Buffer& buff);

    //与指定的io收发数据
    ssize_t read_from_fd(int fd, int* error);
    ssize_t write_to_fd(int fd, int* error);

private:
    //获取完整缓存空间的首地址
    char* get_begin_ptr();
    const char* get_begin_ptr() const;
    //缓存空间扩容
    void make_space(size_t len);

private:    
    std::vector<char> buffer_; //buffer底层数据结构
    std::atomic<std::size_t> read_pos_;
    std::atomic<std::size_t> write_pos_;
};

#endif // !__BUFFER_H_