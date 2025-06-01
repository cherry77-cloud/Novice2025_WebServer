#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <unordered_map>
#include <string>
#include <string_view>

#include "buffer.h"
#include "cgi_handler.h"
#include "unique_fd.h"
#include "date_cache.h"

class HTTPresponse {
public:
    HTTPresponse();
    ~HTTPresponse();

    void init(std::string_view srcDir, std::string_view path, bool isKeepAlive = false, int code = -1);
    void makeResponse(Buffer& buffer);
    void makeCGIResponse(Buffer& buffer, const std::string& method, const std::string& body, const std::string& queryString);
    void unmapFile_();
    char* file();
    size_t fileLen() const;
    void errorContent(Buffer& buffer, std::string_view message);
    
    // 高性能sendfile方法，使用TCP_CORK优化
    ssize_t sendFileOptimized(int sockFd, const Buffer& headerBuffer);

private:
    void addStateLine_(Buffer& buffer);
    void addResponseHeader_(Buffer& buffer);
    void addContent_(Buffer& buffer);

    void errorHTML_();
    std::string_view getFileType_();

    int code_;
    bool isKeepAlive_;

    std::string path_;
    std::string srcDir_;

    char* mmFile_;
    struct stat mmFileStat_;

    unique_fd fileFd_;  // 使用RAII管理文件描述符
    
    // CGI处理器
    static CGIHandler cgiHandler_;

    static const std::unordered_map<std::string_view, std::string_view> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string_view> CODE_STATUS;
    static const std::unordered_map<int, std::string_view> CODE_PATH;
};

#endif  // HTTP_RESPONSE_H