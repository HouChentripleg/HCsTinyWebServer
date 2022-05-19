#include "../include/httpconnect.hpp"

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userNum;
bool HttpConn::isET;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HttpConn::~HttpConn() {
    closeConn();
}

void HttpConn::initConn(int sockfd, const sockaddr_in& addr) {
    assert(sockfd > 0);
    userNum++;
    addr_ = addr;
    fd_ = sockfd;
    writeBuffer_.RetrieveAll();
    readBuffer_.RetrieveAll();
    isClose_ = false;
}

void HttpConn::closeConn() {
    response_.unmapFile();  // 取消映射
    if(isClose_ == false) {
        isClose_ = true;
        userNum--;
        close(fd_);
    }
}

int HttpConn::writeBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}

ssize_t HttpConn::readBuffer(int *saveError) {
    ssize_t len = -1;
    // 一次性读出所有数据
    do {
        len = readBuffer_.ReadFd(fd_, saveError);
        if(len <= 0) {
            break;
        }
    } while(isET);   // 是ET就一直读
    return len;
}

ssize_t HttpConn::writeBuffer(int *saveError) {
    ssize_t len = -1;
    do {
        // 分散写数据
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveError = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len == 0) {
            // 缓存为空，数据传输结束
            break;
        } else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            // 写到第二个vector I/O
            // 更新响应头传输起点、长度和缓冲区
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                // 状态行+响应头已传输完成，不再需要
                writeBuffer_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        } else {
            // 还没写到第二个vector I/O
            // 更新响应头传输起点、长度和缓冲区
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuffer_.Retrieve(len);
        }
    } while(isET || writeBytes() > 10240);  // 一次最多传输10MB数据
    return len;
}

bool HttpConn::handleConn() {
    request_.init();  // 初始化请求对象
    if(readBuffer_.readableBytes() <= 0) {
        //没有请求数据
        return false;
    } else if(request_.parse(readBuffer_)) {
        // 解析请求数据，初始化响应对象
        response_.init(srcDir, request_.path(), request_.isKeepAlive(), 200);
    } else {
        // 解析请求数据失败
        std::cout << "400!" << std::endl;
        response_.init(srcDir, request_.path(), false, 400);
    }

    // 生成响应数据
    response_.makeResponse(writeBuffer_);
    // 响应行+头(第一个vector I/O)：指向writeBuffer_
    iov_[0].iov_base = const_cast<char *>(writeBuffer_.curReadPtr());
    iov_[0].iov_len = writeBuffer_.readableBytes();
    iovCnt_ = 1;
    // 响应体(第二个vector I/O)：指向内存映射的文件
    if(response_.file() && response_.fileLength() > 0) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLength();
        iovCnt_ = 2;
    }
    return true;
}