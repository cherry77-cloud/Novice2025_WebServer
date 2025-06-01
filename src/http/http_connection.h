#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <arpa/inet.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <atomic>

#include "http_request.h"
#include "http_response.h"
#include "buffer.h"

class HTTPconnection {
public:
    HTTPconnection();
    ~HTTPconnection();

    void initHTTPConn(int socketFd, const sockaddr_in& addr);

    ssize_t readBuffer(int* saveErrno);
    ssize_t writeBuffer(int* saveErrno);

    void closeHTTPConn();
    bool handleHTTPConn();

    int getFd() const;
    struct sockaddr_in getAddr() const;

    int writeBytes() {
        return iov_[1].iov_len + iov_[0].iov_len;
    }

    bool isKeepAlive() const {
        return request_.isKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    struct sockaddr_in addr_;
    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuffer_;
    Buffer writeBuffer_;

    HTTPrequest request_;
    HTTPresponse response_;
};

#endif  // HTTP_CONNECTION_H