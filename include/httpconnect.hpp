#ifndef HTTP_CONNECT_H
#define HTTP_CONNECT_H

#include <atomic>
#include <arpa/inet.h>
#include "httprequest.hpp"
#include "httpresponse.hpp"
#include "buffer.hpp"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void initConn(int sockfd, const sockaddr_in& addr);  // 初始化该连接 
    void closeConn();   // 关闭该连接

    // 读写接口
    ssize_t readBuffer(int *saveError);
    ssize_t writeBuffer(int *saveError);

    bool handleConn();  // 业务逻辑：解析request并生成response

    // 获取连接的信息
    const char* getIP() const { return inet_ntoa(addr_.sin_addr); };
    int getPort() const { return addr_.sin_port; };
    int getFd() const { return fd_; };
    sockaddr_in getAddr() const { return addr_; };

    int writeBytes();   // 获取已写入的数据长度
    bool isKeepAlive() { return request_.isKeepAlive(); };

    static bool isET;   // 边缘触发or水平触发
    static const char* srcDir;  // 目录路径
    static std::atomic<int> userNum;    // 用户数量
private:
    int fd_;    // HTTP连接对应的fd
    struct sockaddr_in addr_;   // client的地址
    bool isClose_;   // 是否关闭HTTP连接
    
    int iovCnt_;    // writev()参数
    struct iovec iov_[2];   // vector I/O

    Buffer readBuffer_; // 读缓冲区
    Buffer writeBuffer_;// 写缓冲区

    HttpRequest request_;
    HttpResponse response_;    
};

#endif