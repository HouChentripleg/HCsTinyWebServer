#ifndef MY_WEB_SERVER_H
#define MY_WEB_SERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "epoll.hpp"
#include "timer.hpp"
#include "threadpool.hpp"
#include "httpconnect.hpp"

class WebServer {
public:
    WebServer(int port, int trigMode, int timewaitMS, bool isLinger, int threadNum);
    ~WebServer();

    void Start();   // 服务器开始运行

private:
    bool initSocket_(); // 服务器socket初始化
    void initEventMode_(int trigMode);  // 设置不同套接字的触发模式

    void addClientConn_(int fd, sockaddr_in addr);   // 添加一个Http连接
    void closeConn_(HttpConn *client);  // 关闭一个Http连接

    void handleListen_();   // 监听套接字accept http连接，将连接加入epoll实例
    void handleWrite_(HttpConn *client);
    void handleRead_(HttpConn *client);

    void onRead_(HttpConn *client);
    void onWrite_(HttpConn *client);
    void onProcess_(HttpConn *client);

    void sendError_(int fd, const char* info);  // 发送错误
    void extentTime_(HttpConn *client); // 更新定时器

    static const int MAX_FD = 65536;
    
    static int setFdNonblock(int fd);

    int port_;  // 端口
    int timewaitMS_;  // 定时器默认的过期时间
    bool isClose_;  // 服务器是否关闭
    int listenFd_;  // 监听套接字
    bool isLinger_; // 延时关闭
    char *srcDir_;  // 需要获取的资源路径

    uint32_t listenEvent_;  // 监听事件
    uint32_t connectionEvent_;  // 连接事件

    std::unique_ptr<TimerManager> timer_;   // 定时器
    std::unique_ptr<ThreadPool> threadpool_;// 线程池
    std::unique_ptr<Epoll> epoll_;  // Epoll实例
    std::unordered_map<int, HttpConn> users_;   // client连接
};

#endif