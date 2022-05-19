#include "../include/buffer.hpp"

Buffer::Buffer(int initBufferSize) 
    : buffer_(initBufferSize), readPos_(0), writePos_(0) {}

// 可读的字节 = 写位置 - 读位置
size_t Buffer::readableBytes() const {
    return writePos_ - readPos_;
}

// 可写的字节 = buffer总长 - 写位置
size_t Buffer::writeableBytes() const {
    return buffer_.size() - writePos_;
}

// readPos_之前的容器空间可用
size_t Buffer::prependableBytes() const {
    return readPos_;
}

const char * Buffer::curReadPtr() const {
    return BeginPtr_() + readPos_;
}

const char *Buffer::curWritePtrConst() const {
    return BeginPtr_() + writePos_;
}

char *Buffer::curWritePtr() {
    return BeginPtr_() + writePos_;
}

char *Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char *Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}

void Buffer::Retrieve(size_t len) {
    assert(len <= readableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUntill(const char *end) {
    assert(curReadPtr() <= end);
    Retrieve(end - curReadPtr());
}

void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToString() {
    std::string str(curReadPtr(), readableBytes());
    RetrieveAll();
    return str;
}

// str.data()返回指向str的const void*类型指针
void Buffer::Append(const std::string &str) {
    Append(str.data(), str.length());
};

void Buffer::Append(const void *data, size_t len) {
    assert(data);   // 指针非空
    Append(static_cast<const char*>(data), len);
};

void Buffer::Append(const char *str, size_t len) {
    assert(str);
    EnsureWriteable(len);   // len个字节的数据可写入
    std::copy(str, str + len, curWritePtr()); // 追加数据
    UpdateWritePtr(len);
};
    
void Buffer::Append(const Buffer &buffer) {
    Append(buffer.curReadPtr(), buffer.readableBytes());
}

// 可写字节数<待写字节长度，需要增加空间
void Buffer::EnsureWriteable(size_t len) {
    if(writeableBytes() < len) {
        MakeSpace(len);
    }
    assert(writeableBytes() >= len);
}

void Buffer::UpdateWritePtr(size_t len) {
    writePos_ += len;
}

void Buffer::MakeSpace(size_t len) {
    if(writeableBytes() + prependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } else {
        // 把已读数据向头部移动，为后续数据腾出空间
        size_t readable = readableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
        assert(readable == readableBytes());
    }
}

ssize_t Buffer::ReadFd(int fd, int *saveErrno) {
    char stackbuffer[65535]; // 栈上的临时数组   
    struct iovec iov[2];    // vector I/O
    const size_t writeable = writeableBytes();

    // 2个IO口：iov[0]对应缓冲区buffer中可写的后面一部分，iov[1]对应栈上的临时数组
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writeable;
    iov[1].iov_base = stackbuffer;
    iov[1].iov_len = sizeof(stackbuffer);

    // buffer_中空间够用，则使用iov[0]; 不够用，则使用iov[1]
    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    } else if(static_cast<size_t>(len) <= writeable) {
        // buffer_足够用，writePos_右移len
        writePos_ += len;
    } else {
        // buffer_不够用，writePos_移至最右端，把buffer_扩容，将栈中数据追加到buffer_中
        writePos_ = buffer_.size();
        Append(stackbuffer, len - writeable);
    }
    return len;
};

ssize_t Buffer::WriteFd(int fd, int *saveErrno) {
    size_t readable = readableBytes();
    ssize_t len = write(fd, curReadPtr(), readable);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}