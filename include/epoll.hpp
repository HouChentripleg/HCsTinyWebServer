#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <assert.h>

class Epoll {
public:
    Epoll(int maxEvent = 1024);
    ~Epoll();

    // 对fd的操作
    bool addFd(int fd, uint32_t events);    // 将fd加入Epoll实例
    bool modFd(int fd, uint32_t events);    // 修改fd对应的事件
    bool rmFd(int fd);     // 从Epoll实例中移除fd
    // 返回准备就绪的fd集合
    int wait(int timeMS = -1);    // 成功则返回就绪fd的数量

    // 对外接口：返回就绪fd的信息
    int getEventFd(size_t i) const; // 获取就绪的fd
    uint32_t getEvents(size_t i) const; // 获取就绪fd对应的事件

private:
    int epollfd_;   // epoll_create()的返回值
    std::vector<struct epoll_event> events_;  // 检测到的就绪事件集合
};

#endif