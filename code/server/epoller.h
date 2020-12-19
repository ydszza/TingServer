/**
 * @Author:         yds
 * @Date    :       2020-12-18
*/

#ifndef __EPOLLER_H_
#define __EPOLLER_H_

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>


class Epoller {
public:
    explicit Epoller(int max_event = 1024);
    ~Epoller();

    bool add_fd(int fd, uint32_t events);
    bool mod_fd(int fd, uint32_t events);
    bool del_fd(int fd);
    int wait(int timeout_ms = -1);
    int get_event_fd(size_t i) const;
    uint32_t get_events(size_t i) const;

private:
    int epoll_fd_;
    std::vector<struct epoll_event> events_;
};

#endif // !__EPOLLER_H_