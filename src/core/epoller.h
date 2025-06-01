#ifndef EPOLLER_H
#define EPOLLER_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    bool addFd(int fd, uint32_t events);
    bool modFd(int fd, uint32_t events);
    bool delFd(int fd);
    int wait(int timewait = -1);

    int getEventFd(size_t i) const;
    uint32_t getEvents(size_t i) const;

private:
    int epollerFd_;
    std::vector<struct epoll_event> events_;
};

#endif  // EPOLLER_H