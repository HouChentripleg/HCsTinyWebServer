#include "../include/httpresponse.hpp"

// 文件扩展名对应的MIME-TYPE类型(网页/图片/视频...)，在响应头的Content-Type中指定
const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE {
    {".html",  "text/html"},
    {".xml",   "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt",   "text/plain"},
    {".rtf",   "application/rtf"},
    {".pdf",   "application/pdf"},
    {".word",  "application/nsword"},
    {".png",   "image/png"},
    {".gif",   "image/gif"},
    {".jpg",   "image/jpg"},
    {".au",    "audio/basic"},
    {".mpeg",  "video/mpeg"},
    {".mpg",   "video/mpeg"},
    {".avi",   "video/x-msvideo"},
    {".gz",    "application/x-gzip"},
    {".tar",   "application/x-tar"},
    {".css",   "text/css"},
    {".js",    "text/javascript"}
};

// 状态码对应的服务器应答的状态
const std::unordered_map<int, std::string> HttpResponse::CODE_STATE {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

// 状态码对应的资源路径
const std::unordered_map<int, std::string> HttpResponse::CODE_PATH {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() 
    : stateCode_(-1), isKeepAlive_(false), path_(""), srcDir_(""), mmapFile_(nullptr), mmapFileStat_({0}) {}

HttpResponse::~HttpResponse() {
    unmapFile();
}

void HttpResponse::unmapFile() {
    if(mmapFile_) {
        munmap(mmapFile_, mmapFileStat_.st_size);
        mmapFile_ = nullptr;
    }
}

void HttpResponse::init(const std::string& srcDir, std::string& path, bool isKeepAlive, int stateCode) {
    assert(srcDir != "");

    if(mmapFile_) {
        unmapFile();
    }

    stateCode_ = stateCode;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmapFile_ = nullptr;
    mmapFileStat_ = {0};
}

void HttpResponse::makeResponse(Buffer& buffer) {
    // 判断请求的资源文件
    if(stat((srcDir_ + path_).data(), &mmapFileStat_) < 0 || S_ISDIR(mmapFileStat_.st_mode)) {
        // srcDir_ + path_文件状态获取失败or文件是目录
        stateCode_ = 404;
    } else if(!(mmapFileStat_.st_mode & S_IROTH)) {
        // 其他人对文件没有读权限
        stateCode_ = 403;
    } else if(stateCode_ == -1) {
        stateCode_ = 200;
    }

    errorHTML();
    addStateLine(buffer);
    addResponseHeader(buffer);
    addResponseContent(buffer);
}

void HttpResponse::errorHTML() {
    if(CODE_PATH.count(stateCode_) == 1) {
        // 对应CODE_PATH中的一种错误状态
        path_ = CODE_PATH.find(stateCode_)->second; // 状态码对应的路径
        stat((srcDir_ + path_).data(), &mmapFileStat_);
    }
}

void HttpResponse::addStateLine(Buffer& buffer) {
    std::string status;
    if(CODE_STATE.count(stateCode_) == 1) {
        status = CODE_STATE.find(stateCode_)->second;
    } else {
        stateCode_ = 400;
        status = CODE_STATE.find(stateCode_)->second;
    }
    buffer.Append("HTTP/1.1 " + std::to_string(stateCode_) + " " + status + "\r\n");
}

void HttpResponse::addResponseHeader(Buffer& buffer) {
    buffer.Append("Connection: ");
    if(isKeepAlive_) {
        buffer.Append("keep-alive\r\n");
        // 长链接最多接收6次请求就断开，超时时间120s
        buffer.Append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buffer.Append("close\r\n");
    }
    buffer.Append("Content-type: " + getFileType() + "\r\n");
}

void HttpResponse::addResponseContent(Buffer& buffer) {
    // 打开srcDir_ + path_指定的文件：只读
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcFd < 0) {
        errorContent(buffer, "File Not Found, Open Failed!");
        return;
    }

    // 将文件映射到内存，采用写时拷贝的私有映射，对其他进程不可见，返回内存映射的首地址
    void *mmapAddr = mmap(NULL, mmapFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(mmapAddr == MAP_FAILED) {
        errorContent(buffer, "File Not Found, MMAP Failed!");
        return;
    }
    mmapFile_ = (char *)mmapAddr;
    close(srcFd);
    buffer.Append("Content-length: " + std::to_string(mmapFileStat_.st_size) + "\r\n\r\n");
}

std::string HttpResponse::getFileType() {
    std::string::size_type idx = path_.find_last_of('.');   // 找到路径中最后一个'.'所在的位置
    if(idx == std::string::npos) {
        // 不存在'.'
        return "text/plain";
    }
    std::string suffix = path_.substr(idx); // 获取文件的扩展名
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::errorContent(Buffer& buffer, std::string message) {
    std::string body;
    std::string status;

    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";    // 白色背景色

    if(CODE_STATE.count(stateCode_) == 1) {
        status = CODE_STATE.find(stateCode_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(stateCode_) + " : " + status + "\n";
    body += "<p>" + message +"</p>";
    body += "<hr><em>HCsTinyWebServer</em></body></html>";

    buffer.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buffer.Append(body);
}