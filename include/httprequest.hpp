#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <regex>
#include "buffer.hpp"

class HttpRequest {
public:
    // 解析状态
    enum ParseState {
        RequestLine,    // 请求行
        Header, // 请求头部
        Body,   // 请求体
        Finish, // 完成
    };

    HttpRequest() {
        init();
    }
    ~HttpRequest() = default;

    void init();    // 初始化HttpRequest类的对象的数据
    
    bool parse(Buffer &buffer); // 解析从缓冲区中读到的数据

    // 获取HTTP信息
    std::string path() const;   // 获取路径
    std::string& path();
    std::string method() const; // 获取请求方法
    std::string version() const;// 获取协议版本
    std::string GetPost(const std::string& key) const; // POST方式下获取key对应的value
    std::string GetPost(const char* key) const;
    bool isKeepAlive() const;   // 连接是否keep alive

private:
    // 解析HTTP请求
    bool ParseRequestLine(const std::string& line);
    void ParseHeader(const std::string& line);
    void ParseBody(const std::string& line);
    
    void ParsePath();   // 解析请求资源的路径
    void ParsePost();   // 若请求体的格式为POST则解析POST报文

    static int ConvertHex(char ch); // 16进制字符转10进制整数

    ParseState parse_state_;    // 解析状态
    std::string method_, path_,version_, body_; // 请求方法，路径，协议版本，请求体
    std::unordered_map<std::string, std::string> header_;   // 请求头部 <key>:<value>
    std::unordered_map<std::string, std::string> post_;     // POST请求表单数据

    static const std::unordered_set<std::string> DEFAULT_HTML;  // 默认网页
};

#endif