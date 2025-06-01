#include "http_request.h"
#include <algorithm>  // 添加std::search支持

void HTTPrequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HTTPrequest::isKeepAlive() const {
    auto it = header_.find("Connection");
    if (it != header_.end()) {
        return it->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HTTPrequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if (buff.readableBytes() <= 0) {
        return false;
    }
    while (buff.readableBytes() && state_ != FINISH) {
        const char* lineEnd =
            std::search(buff.curReadPtr(), buff.curWritePtrConst(), CRLF, CRLF + 2);
        std::string_view line(buff.curReadPtr(), lineEnd - buff.curReadPtr());
        switch (state_) {
            case REQUEST_LINE:
                if (!parseRequestLine_(line)) {
                    return false;
                }
                parsePath_();
                break;
            case HEADERS:
                parseRequestHeader_(line);
                if (buff.readableBytes() <= 2) {
                    state_ = FINISH;
                }
                break;
            case BODY:
                parseDataBody_(line);
                break;
            default:
                break;
        }
        if (lineEnd == buff.curWritePtr()) {
            break;
        }
        buff.updateReadPtrUntilEnd(lineEnd + 2);
    }
    return true;
}

void HTTPrequest::parsePath_() {
    // API路由处理
    if (path_.find("/api/") == 0) {
        // 保持API路径不变，不添加.html后缀
        return;
    }
    
    // 简化的HTML路由处理
    if (path_ == "/") {
        path_ = "/index.html";
    } else if (path_ == "/index") {
        path_ += ".html";
    }
}

// 高性能手写解析 - 替代regex
bool HTTPrequest::parseRequestLine_(std::string_view line) {
    // 解析格式: METHOD PATH HTTP/VERSION
    size_t first_space = findChar(line, ' ');
    if (first_space == std::string_view::npos) return false;
    
    size_t second_space = findChar(line, ' ', first_space + 1);
    if (second_space == std::string_view::npos) return false;
    
    // 检查HTTP前缀
    if (line.substr(second_space + 1, 5) != "HTTP/") return false;
    
    method_ = line.substr(0, first_space);
    path_ = line.substr(first_space + 1, second_space - first_space - 1);
    version_ = line.substr(second_space + 6); // 跳过"HTTP/"
    
    state_ = HEADERS;
    return true;
}

void HTTPrequest::parseRequestHeader_(std::string_view line) {
    if (line.empty()) {
        state_ = BODY;
        return;
    }
    
    size_t colon_pos = findChar(line, ':');
    if (colon_pos == std::string_view::npos) {
        state_ = BODY;
        return;
    }
    
    std::string key(line.substr(0, colon_pos));
    std::string_view value = line.substr(colon_pos + 1);
    
    // 跳过值前面的空格
    while (!value.empty() && value.front() == ' ') {
        value.remove_prefix(1);
    }
    
    // 存储为string避免悬垂引用
    header_[key] = std::string(value);
}

void HTTPrequest::parseDataBody_(std::string_view line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
}

void HTTPrequest::parsePost_() {
    auto content_type_it = header_.find("Content-Type");
    if (method_ == "POST" && content_type_it != header_.end() && 
        content_type_it->second == "application/x-www-form-urlencoded") {
        
        if (body_.size() == 0) {
            return;
        }

        // 解析POST数据并存储为string
        std::string_view body_view = body_;
        size_t start = 0;
        
        while (start < body_view.size()) {
            size_t eq_pos = body_view.find('=', start);
            if (eq_pos == std::string_view::npos) break;
            
            size_t amp_pos = body_view.find('&', eq_pos + 1);
            if (amp_pos == std::string_view::npos) amp_pos = body_view.size();
            
            std::string key(body_view.substr(start, eq_pos - start));
            std::string value(body_view.substr(eq_pos + 1, amp_pos - eq_pos - 1));
            
            post_[key] = value;
            start = amp_pos + 1;
        }
    }
}

// 快速字符查找辅助函数
size_t HTTPrequest::findChar(std::string_view str, char ch, size_t start) {
    for (size_t i = start; i < str.size(); ++i) {
        if (str[i] == ch) return i;
    }
    return std::string_view::npos;
}

std::string HTTPrequest::path() const {
    return path_;
}

std::string& HTTPrequest::path() {
    return path_;
}

const std::string& HTTPrequest::path_ref() const {
    return path_;
}

std::string HTTPrequest::method() const {
    return method_;
}

std::string HTTPrequest::version() const {
    return version_;
}

std::string HTTPrequest::getPost(const std::string& key) const {
    assert(key != "");
    auto it = post_.find(key);
    if (it != post_.end()) {
        return it->second;
    }
    return "";
}

std::string HTTPrequest::getPost(const char* key) const {
    assert(key != nullptr);
    auto it = post_.find(std::string(key));
    if (it != post_.end()) {
        return it->second;
    }
    return "";
}

std::string HTTPrequest::getBody() const {
    return body_;
}