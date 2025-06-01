#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <string_view>
#include <unordered_map>

#include "buffer.h"

class HTTPrequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HTTPrequest() {
        init();
    };
    ~HTTPrequest() = default;

    void init();
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    const std::string& path_ref() const;  // 新增避免拷贝的版本
    std::string method() const;
    std::string version() const;
    std::string getPost(const std::string& key) const;
    std::string getPost(const char* key) const;
    std::string getBody() const;

    bool isKeepAlive() const;

private:
    bool parseRequestLine_(std::string_view line);    // 使用string_view优化
    void parseRequestHeader_(std::string_view line);  // 使用string_view优化
    void parseDataBody_(std::string_view line);       // 使用string_view优化

    void parsePath_();
    void parsePost_();

    // 快速字符串查找辅助函数
    static size_t findChar(std::string_view str, char ch, size_t start = 0);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    
    // 改回string以避免悬垂引用（Buffer在keep-alive时会被清空）
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
};

#endif  // HTTP_REQUEST_H