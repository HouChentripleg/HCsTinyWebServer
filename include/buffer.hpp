#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <cstring>
#include <assert.h>
#include <sys/uio.h>

class Buffer {
public:
    Buffer(int initBufferSize = 1024);
    ~Buffer() = default;

    // 获取缓冲区信息的接口
    size_t readableBytes() const; // 可读的字节长度
    size_t writeableBytes() const;// 可写的字节长度
    size_t prependableBytes() const;// 头部预留字节长度

    // 缓冲区与客户端fd的读写接口
    ssize_t ReadFd(int fd, int *Errno); // 将数据从fd读入缓冲区
    ssize_t WriteFd(int fd, int *Errno);// 将数据从缓冲区写入fd

    // 缓冲区与应用程序间的读写接口
    // 程序从缓冲区中读入数据
    void Retrieve(size_t len);  // 从buffer中读取len个字节
    void RetrieveUntill(const char *end); // 读取到指定地址end
    void RetrieveAll(); // 读完缓冲区中的所有字节
    std::string RetrieveAllToString();  // 将可读数据转成string
    // 程序向缓冲区追加数据
    void Append(const std::string &str);
    void Append(const char *str, size_t len);
    void Append(const void *data, size_t len);
    void Append(const Buffer &buffer);
    void EnsureWriteable(size_t len); // 确保长度为len的数据可写入
    void UpdateWritePtr(size_t len);  // 修改写后的索引位置

    // 获取当前的读写指针(地址)
    const char *curReadPtr() const;
    char *curWritePtr();
    const char *curWritePtrConst() const;
    
private:
    char *BeginPtr_();  // 获取缓冲区的内存起始地址
    const char *BeginPtr_() const;
    void MakeSpace(size_t len); // 扩充缓冲区空间

    std::vector<char> buffer_;  // 应用层的读写缓冲区
    std::atomic<size_t> readPos_;  // 读的位置(缓冲区中的索引)  //原子变量，保证数据访问的互斥
    std::atomic<size_t> writePos_;  // 写的位置(索引)
};

#endif