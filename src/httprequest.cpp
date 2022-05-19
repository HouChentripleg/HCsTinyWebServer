#include "../include/httprequest.hpp"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

void HttpRequest::init() {
    parse_state_ = RequestLine;
    path_ = method_ = version_ = body_ = "";
    header_.clear();
    post_.clear();
}

// Connection:keep-alive键值对<key>:<value>
bool HttpRequest::isKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer &buffer) {
    const char *CRLF = "\r\n";  // 回车换行符
    if(buffer.readableBytes() <= 0) {
        return false;
    }
    // buffer中有可读数据&&解析状态未到Finish，就一直解析
    while(buffer.readableBytes() && parse_state_ != Finish) {
        // 获取每一行，以\r\n为结束标志，lineEnd指向\r
        const char* lineEnd = std::search(buffer.curReadPtr(), buffer.curWritePtrConst(), CRLF, CRLF + 2);
        std::string line(buffer.curReadPtr(), lineEnd);
        switch (parse_state_)
        {
        case RequestLine:
            // 解析请求行
            if(!ParseRequestLine(line)) {
                return false;
            }
            ParsePath();
            break;
        case Header:
            // 解析请求头部
            ParseHeader(line);
            if(buffer.readableBytes() <= 2) {
                // 缓冲区无可读数据
                parse_state_ = Finish;
            }
            break;
        case Body:
            // 解析请求体
            ParseBody(line);
            break;
        default:
            break;
        }
        if(lineEnd == buffer.curWritePtr()) {
            // \r即为buffer中最后写入的数据
            break;
        }
        // 读取到\n的下一位
        buffer.RetrieveUntill(lineEnd + 2);
    }
    return true;
}

std::string HttpRequest::path() const {
    return path_;
}

std::string& HttpRequest::path() {
    return path_;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

void HttpRequest::ParsePath() {
    if(path_ == "/") {
        // 若访问根目录，默认访问index.html
        // 例如http://192.168.157.128:10000/
        path_ = "/index.html";
    } else {
        // 访问默认的其他页面
        // 例如http://192.168.157.128:10000/picture
        for(auto &item : DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

// 解析请求行
bool HttpRequest::ParseRequestLine(const std::string& line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        parse_state_ = Header;
        return true;
    }
    return false;
}

// 解析请求头部
void HttpRequest::ParseHeader(const std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        // 解析到空行，parse_state_转为解析请求体
        parse_state_ = Body;
    }
}

// 解析请求体
void HttpRequest::ParseBody(const std::string& line) {
    body_ = line;
    ParsePost();
    parse_state_ = Finish;
}

// 解析POST报文
void HttpRequest::ParsePost() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        // 解析表单信息
        int n = body_.size();
        if( n == 0) {
            return;
        }
        std::string key, value;
        int num = 0, i = 0, j = 0;
        for(; i < n; i++) {
            char ch = body_[i];
            switch (ch)
            {
            case '=':
                key = body_.std::string::substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                body_[i] = ' ';
                break;
            case '%':
                num = ConvertHex(body_[i + 1]) * 16 + ConvertHex(body_[i + 2]);
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = body_.std::string::substr(j, i - j);
                j = i + 1;
                post_[key] = value; // 添加到post_表单数据中
                break;
            default:
                break;
            }
        }
        assert(j <= i);
        // 最后一对key=value，其后没有&分割符
        if(post_.count(key) == 0 && j < i) {
            value = body_.std::string::substr(j, i - j);
            post_[key] = value;
        }
    }
}

int HttpRequest::ConvertHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
}
