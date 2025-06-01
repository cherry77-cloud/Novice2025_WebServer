#ifndef BUFFER_H
#define BUFFER_H

#include <assert.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <vector>
#include <string_view>

class Buffer {
public:
    Buffer(int initBufferSize = 8192);  // 提升到8KB避免早期重新分配
    ~Buffer() = default;
    
    // 移动构造和移动赋值
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;
    
    // 禁用拷贝
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    
    size_t writeableBytes() const;
    size_t readableBytes() const;

    const char* curReadPtr() const;
    const char* curWritePtrConst() const;
    char* curWritePtr();
    void updateReadPtr(size_t len);
    void updateReadPtrUntilEnd(const char* end);
    void updateWritePtr(size_t len);
    void initPtr();

    void ensureWriteable(size_t len);
    void append(const char* str, size_t len);
    void append(const char* str);  // 添加const char*版本避免歧义
    void append(const std::string& str);
    void append(std::string_view str);  // 新增string_view支持
    void append(const void* data, size_t len);
    void append(Buffer&& buffer);  // 移动语义版本

    ssize_t readFd(int fd, int* Errno);
    ssize_t writeFd(int fd, int* Errno);

    std::string AlltoStr();
    std::string_view view() const;  // 获取string_view避免拷贝

private:
    char* BeginPtr_();
    const char* BeginPtr_() const;
    void allocateSpace(size_t len);

    std::vector<char> buffer_;
    size_t readPos_;   // 改为普通size_t，提升性能
    size_t writePos_;  // 改为普通size_t，提升性能
};

#endif  // BUFFER_H