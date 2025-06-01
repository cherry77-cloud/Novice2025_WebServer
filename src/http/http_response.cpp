#include "http_response.h"
#include <sys/sendfile.h>
#include <algorithm>  // for std::lower_bound

const std::unordered_map<std::string_view, std::string_view> HTTPresponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpv",   "video/mpv" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css" },
    { ".js",    "text/javascript" },
};

const std::unordered_map<int, std::string_view> HTTPresponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string_view> HTTPresponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HTTPresponse::HTTPresponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isKeepAlive_ = false;
    mmFile_ = nullptr;
    mmFileStat_ = {0};
};

HTTPresponse::~HTTPresponse() {
    unmapFile_();
}

void HTTPresponse::init(std::string_view srcDir, std::string_view path, bool isKeepAlive, int code) {
    if (mmFile_) {
        unmapFile_();
    }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;  // 延迟拷贝，只在需要时才转为string
    srcDir_ = srcDir;  // 延迟拷贝
    mmFile_ = nullptr;
    mmFileStat_ = {0};
}

void HTTPresponse::makeResponse(Buffer& buff) {
    if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    } else if (!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    } else if (code_ == -1) {
        code_ = 200;
    }
    errorHTML_();
    addStateLine_(buff);
    addResponseHeader_(buff);
    addContent_(buff);
}

char* HTTPresponse::file() {
    return mmFile_;
}

size_t HTTPresponse::fileLen() const {
    return mmFileStat_.st_size;
}

void HTTPresponse::errorHTML_() {
    if (CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

void HTTPresponse::addStateLine_(Buffer& buffer) {
    std::string_view status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buffer.append("HTTP/1.1 " + std::to_string(code_) + " ");
    buffer.append(status.data(), status.size());
    buffer.append("\r\n");
}

void HTTPresponse::addResponseHeader_(Buffer& buffer) {
    buffer.append("Connection: ");
    if(isKeepAlive_) {
        buffer.append("keep-alive\r\n");
    } else{
        buffer.append("close\r\n");
    }
    buffer.append("Content-Type: ");
    auto file_type = getFileType_();
    buffer.append(file_type.data(), file_type.size());
    buffer.append("\r\n");
    
    // 使用缓存的Date头，避免每次strftime调用
    buffer.append(getCachedDateHeader());
}

void HTTPresponse::addContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if (srcFd < 0) {
        errorContent(buff, "File NotFound!");
        return;
    }
    
    fileFd_.reset(srcFd);  // 使用RAII管理
    
    // 内存映射文件
    mmFile_ = (char*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (mmFile_ == MAP_FAILED) {
        errorContent(buff, "File mmap error!");
        return;
    }
    
    buff.append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HTTPresponse::unmapFile_() {
    fileFd_.close();  // RAII自动管理
    if (mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

std::string_view HTTPresponse::getFileType_() {
    // 使用thread_local LRU缓存，避免每个对象存储缓存
    thread_local static std::unordered_map<std::string, std::string_view> cache;
    
    std::string::size_type idx = path_.find_last_of('.');
    if (idx == std::string::npos) {
        return "text/plain";
    }
    
    std::string suffix(path_.data() + idx, path_.size() - idx);
    
    // 查找缓存
    auto cache_it = cache.find(suffix);
    if (cache_it != cache.end()) {
        return cache_it->second;
    }
    
    // 查找MIME类型
    auto it = SUFFIX_TYPE.find(suffix);
    std::string_view mime_type = (it != SUFFIX_TYPE.end()) ? it->second : "text/plain";
    
    // 简单LRU：限制缓存大小
    if (cache.size() >= 128) {
        cache.clear();  // 简单清空策略
    }
    cache[suffix] = mime_type;
    
    return mime_type;
}

// 静态CGI处理器实例
CGIHandler HTTPresponse::cgiHandler_;

void HTTPresponse::makeCGIResponse(Buffer& buffer, const std::string& method, const std::string& body, const std::string& queryString) {
    // 使用CGI处理器处理请求
    cgiHandler_.handleCGI(path_, method, body, queryString, buffer);
}

void HTTPresponse::errorContent(Buffer& buff, std::string_view message) {
    std::string body;
    std::string_view status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status.data() + "\n";
    body += "<p>" + std::string(message.data()) + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}

// 高性能TCP_CORK + sendfile优化
ssize_t HTTPresponse::sendFileOptimized(int sockFd, const Buffer& headerBuffer) {
    if (!fileFd_) {
        return 0;
    }
    
    ssize_t totalSent = 0;
    off_t offset = 0;
    ssize_t remain = mmFileStat_.st_size;
    
    // 1. 启用TCP_CORK，聚合小包
    int cork = 1;
    if (setsockopt(sockFd, SOL_TCP, TCP_CORK, &cork, sizeof(cork)) < 0) {
        // TCP_CORK失败时回退到普通方式
        return -1;
    }
    
    // 2. 发送HTTP头
    const char* headerData = headerBuffer.curReadPtr();
    size_t headerSize = headerBuffer.readableBytes();
    ssize_t headerSent = send(sockFd, headerData, headerSize, MSG_NOSIGNAL);
    if (headerSent < 0) {
        // 禁用TCP_CORK
        cork = 0;
        setsockopt(sockFd, SOL_TCP, TCP_CORK, &cork, sizeof(cork));
        return headerSent;
    }
    totalSent += headerSent;
    
    // 3. 使用sendfile发送文件内容 - 零拷贝
    while (remain > 0) {
        ssize_t sent = sendfile(sockFd, fileFd_.get(), &offset, remain);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;  // 重试
            }
            break;
        }
        remain -= sent;
        totalSent += sent;
    }
    
    // 4. 禁用TCP_CORK，立即发送
    cork = 0;
    setsockopt(sockFd, SOL_TCP, TCP_CORK, &cork, sizeof(cork));
    
    return totalSent;
}