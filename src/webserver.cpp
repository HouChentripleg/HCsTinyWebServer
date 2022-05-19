#include "../include/webserver.hpp"

WebServer::WebServer(int port, int trigMode, int timewaitMS, bool isLinger, int threadNum) 
    : port_(port), timewaitMS_(timewaitMS), isLinger_(isLinger), isClose_(false), 
    timer_(new TimerManager()), threadpool_(new ThreadPool(threadNum)), epoll_(new Epoll()) {

    srcDir_ = getcwd(nullptr, 256); // 获取当前工作路径
    assert(srcDir_);

    strncat(srcDir_, "/../resources/", 16); // 拼接路径

    // 初始化Http连接
    HttpConn::userNum = 0;
    HttpConn::srcDir = srcDir_;

    initEventMode_(trigMode);
    if(!initSocket_()) {
        // 初始化服务器socket失败
        isClose_ = true;
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
}

void WebServer::initEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;  // 监听事件：仅作初始化，无它用
    connectionEvent_ = EPOLLRDHUP || EPOLLONESHOT;  // 连接事件：对端断开，设置oneshot
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connectionEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    }
    // 判断Http连接是否为ET模式
    HttpConn::isET = (connectionEvent_ & EPOLLET);
}

void WebServer::Start() {
    int timeMS = -1;
    if(!isClose_) {
        std::cout << "====================";
        std::cout<< "HCsTinyWebServer Start!";
        std::cout << "====================" << std::endl;
    }
    // Epoll一直监听事件是否就绪
    while(!isClose_) {
        if(timewaitMS_ > 0) {
            // 清理过期连接，返回下一次处理的时长
            timeMS = timer_->getNextHandle();
        }
        // 在计时器超时前唤醒epoll，判断是否有新事件到达
        int eventCnt = epoll_->wait(timeMS);  // 返回就绪fd的数量
        for(int i = 0; i < eventCnt; i++) {
            int fd = epoll_->getEventFd(i);   // 获取就绪的fd
            uint32_t events = epoll_->getEvents(i); // 获取就绪fd对应的事件

            if(fd == listenFd_) {
                // 监听
                handleListen_();
            } else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 客户端关闭连接
                assert(users_.count(fd) > 0);
                closeConn_(&users_[fd]);
            } else if(events & EPOLLIN) {
                // 读事件
                assert(users_.count(fd) > 0);
                handleRead_(&users_[fd]);
            } else if(events & EPOLLOUT) {
                // 写事件
                assert(users_.count(fd) > 0);
                handleWrite_(&users_[fd]);
            } else {
                // 其他事件
                std::cout << "Unexpected event" << std::endl;
            }
        }
    }
}

void WebServer::sendError_(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, sizeof(info), 0);
    close(fd);
}

void WebServer::closeConn_(HttpConn *client) {
    assert(client);
    epoll_->rmFd(client->getFd());
    client->closeConn();
}

void WebServer::addClientConn_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].initConn(fd, addr);
    if(timewaitMS_ > 0) {
        // 添加定时器，到期关闭连接
        timer_->addTimer(fd, timewaitMS_, std::bind(&WebServer::closeConn_, this, &users_[fd]));
    }
    epoll_->addFd(fd, EPOLLIN | connectionEvent_);
    setFdNonblock(fd);
}

void WebServer::handleListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        if(fd <= 0) {
            return;
        } else if (HttpConn::userNum >= MAX_FD) {
            sendError_(fd, "Server ist beshäftigt! Clients ist voll!");
            return;
        }
        addClientConn_(fd, addr);
    } while(listenFd_ & EPOLLET);
}

void WebServer::handleWrite_(HttpConn *client) {
    assert(client);
    extentTime_(client);
    threadpool_->enqueue(std::bind(&WebServer::onWrite_, this, client));    // 加入线程池任务队列
}

void WebServer::handleRead_(HttpConn *client) {
    assert(client);
    extentTime_(client);
    threadpool_->enqueue(std::bind(&WebServer::onRead_, this, client));
}

void WebServer::extentTime_(HttpConn *client) {
    assert(client);
    if(timewaitMS_ > 0) {
        timer_->update(client->getFd(), timewaitMS_);
    }
}

// 读函数：先接收再处理
void WebServer::onRead_(HttpConn *client) {
    assert(client);
    int ret = -1;
    int readError = 0;
    ret = client->readBuffer(&readError);
    if(ret <= 0 && (readError != EAGAIN || readError != EWOULDBLOCK)) {
        // 摒除无数据可读的错误号(EAGAIN or EWOULDBLOCK)
        closeConn_(client);
        return;
    }
    onProcess_(client);
}

/*  处理函数：判断读入的请求报文是否完整，决定继续监听读事件还是监听写事件
    若请求报文不完整，继续读；请求报文完整，则根据请求内容生成相应的请求响应报文并发送
*/
void WebServer::onProcess_(HttpConn *client) {
    if(client->handleConn()) {
        // 请求报文完整
        epoll_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
    } else {
        // 请求报文不完整，继续读
        epoll_->modFd(client->getFd(), connectionEvent_ | EPOLLIN);
    }
}

// 写函数：发送响应报文
void WebServer::onWrite_(HttpConn *client) {
    assert(client);
    int ret = -1;
    int writeError = 0;
    ret = client->writeBuffer(&writeError);
    if(client->writeBytes() == 0) {
        // 数据已发送完毕
        if(client->isKeepAlive()) {
            onProcess_(client);
            return;
        }
    } else if(ret < 0) {
        // 发送失败
        if(writeError == EAGAIN || writeError == EWOULDBLOCK) {
            // 缓存已满导致发送失败，继续监听写事件
            epoll_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
            return;
        }
    }
    // 其他原因导致发送失败，关闭连接
    closeConn_(client);
}

// 创建并初始化socket：设置socket属性，绑定端口，向epoll注册事件
bool WebServer::initSocket_() {
    struct sockaddr_in addr;
    if(port_ < 1024 || port_ > 65535) {
        std::cout << "Port error! It should be between 1024-65535!" << "\n"; 
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    struct linger optLinger;
    if(isLinger_) {
        // 延迟关闭：直到剩余数据发送完毕or超时
        optLinger.l_onoff = 1;
        optLinger.l_linger = 10;  // 最多等待10s接收客户端关闭确认
    }

    // 创建套接字
    listenFd_ = socket(PF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        return false;
    }

    // 设置延迟关闭
    int ret1 = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret1 < 0) {
        close(listenFd_);
        return false;
    }

    // 端口复用
    int optval = 1;
    int ret2 = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEPORT, (const void*)&optval, sizeof(optval));
    if(ret2 < 0) {
        close(listenFd_);
        return false;
    }

    // 绑定本地IP和port
    int ret3 = bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr));
    if(ret3 < 0) {
        close(listenFd_);
        return false;
    }

    // 设置监听，指定请求队列大小为6
    int ret4 = listen(listenFd_, 6);
    if(ret4 < 0) {
        close(listenFd_);
        return false;
    }

    // 在内核注册Epoll实例，添加监听套接字
    int ret5 = epoll_->addFd(listenFd_, listenEvent_ | EPOLLIN);
    if(ret5 < 0) {
        close(listenFd_);
        return false;
    }

    // 设置监听套接字非阻塞(但是延时关闭还是会导致close()阻塞)
    setFdNonblock(listenFd_);
    return true;
}

int WebServer::setFdNonblock(int fd) {
    assert(fd > 0);
    int oldFlag = fcntl(fd, F_GETFL, 0);
    int newFlag = oldFlag | O_NONBLOCK;
    return fcntl(fd, F_SETFL, newFlag);
}
