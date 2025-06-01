#include "http_connection.h"

const char* HTTPconnection::srcDir;
std::atomic<int> HTTPconnection::userCount;
bool HTTPconnection::isET;

HTTPconnection::HTTPconnection() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
};

HTTPconnection::~HTTPconnection() {
    closeHTTPConn();
};

void HTTPconnection::initHTTPConn(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuffer_.initPtr();
    readBuffer_.initPtr();
    isClose_ = false;
}

void HTTPconnection::closeHTTPConn() {
    response_.unmapFile_();
    if (isClose_ == false) {
        isClose_ = true;
        userCount--;
        close(fd_);
    }
}

int HTTPconnection::getFd() const {
    return fd_;
};

struct sockaddr_in HTTPconnection::getAddr() const {
    return addr_;
}

ssize_t HTTPconnection::readBuffer(int* saveErrno) {
    ssize_t len = -1;
    ssize_t total = 0;
    do {
        len = readBuffer_.readFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
        total += len;
        if (!isET || total > 65536) {
            break;
        }
    } while (true);
    return total > 0 ? total : len;
}

ssize_t HTTPconnection::writeBuffer(int* saveErrno) {
    ssize_t len = -1;
    ssize_t total = 0;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0) {
            *saveErrno = errno;
            break;
        }
        total += len;
        if (iov_[0].iov_len + iov_[1].iov_len == 0) {
            break;
        }
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                writeBuffer_.initPtr();
                iov_[0].iov_len = 0;
            }
        } else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuffer_.updateReadPtr(len);
        }
        if (!isET || total > 65536) {
            break;
        }
    } while (writeBytes() > 0);
    return total > 0 ? total : len;
}

bool HTTPconnection::handleHTTPConn() {
    request_.init();
    if (readBuffer_.readableBytes() <= 0) {
        return false;
    } else if (request_.parse(readBuffer_)) {
        // 检查是否是CGI请求 - 避免路径拷贝
        const std::string& request_path = request_.path_ref();
        if (request_path.find("/cgi-bin/") == 0) {
            // 处理CGI请求
            std::string queryString = "";
            
            // 分离路径和查询字符串
            size_t queryPos = request_path.find('?');
            std::string path_str;
            if (queryPos != std::string::npos) {
                queryString = std::string(request_path.substr(queryPos + 1));
                path_str = std::string(request_path.substr(0, queryPos));
            } else {
                path_str = std::string(request_path);
            }
            
            response_.init(srcDir, path_str, request_.isKeepAlive(), 200);
            response_.makeCGIResponse(writeBuffer_, request_.method(), request_.getBody(), queryString);
            
            // CGI响应不需要文件处理
            iov_[0].iov_base = const_cast<char*>(writeBuffer_.curReadPtr());
            iov_[0].iov_len = writeBuffer_.readableBytes();
            iovCnt_ = 1;
            return true;
        } else {
            // 处理普通HTML请求 - 直接使用string_view
            response_.init(srcDir, request_path, request_.isKeepAlive(), 200);
        }
    } else {
        response_.init(srcDir, request_.path(), false, 400);
    }

    response_.makeResponse(writeBuffer_);
    iov_[0].iov_base = const_cast<char*>(writeBuffer_.curReadPtr());
    iov_[0].iov_len = writeBuffer_.readableBytes();
    iovCnt_ = 1;

    if (response_.fileLen() > 0 && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    return true;
}