#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <unordered_map>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include "buffer.hpp"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int stateCode = -1);  // 响应报文初始化
    void makeResponse(Buffer& buffer);    // 制作响应报文并传送到缓冲区
    void unmapFile();   // 取消文件映射
    
    int code() const { return stateCode_; };    // 获取状态码
    char* file() {return mmapFile_; };   // 获取映射后的文件地址
    size_t fileLength() const {return mmapFileStat_.st_size; };    // 获取映射文件的长度
    void errorContent(Buffer& buffer, std::string message);    // 错误页面

private:
    // 制作HTTP响应报文
    void addStateLine(Buffer& buffer);
    void addResponseHeader(Buffer& buffer);
    void addResponseContent(Buffer& buffer);

    void errorHTML();   // 考虑状态码为40X对应的错误网页
    std::string getFileType();  // 获取文件类型

    int stateCode_; // 响应状态码
    bool isKeepAlive_;

    std::string path_;    // httprequest解析得到的路径
    std::string srcDir_; // 根目录

    char *mmapFile_;    // 文件内存映射的地址
    struct stat mmapFileStat_;  // 文件状态信息

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;  // 后缀名→文件类型
    static const std::unordered_map<int, std::string> CODE_STATE;  // 状态码→状态码描述
    static const std::unordered_map<int, std::string> CODE_PATH;  // 状态码→路径
};

#endif