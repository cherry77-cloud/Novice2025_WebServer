#ifndef UNIQUE_FD_H
#define UNIQUE_FD_H

#include <unistd.h>
#include <utility>

// RAII文件描述符封装，自动管理close()
class unique_fd {
public:
    constexpr unique_fd() noexcept : fd_(-1) {}
    explicit unique_fd(int fd) noexcept : fd_(fd) {}
    
    // 禁用拷贝
    unique_fd(const unique_fd&) = delete;
    unique_fd& operator=(const unique_fd&) = delete;
    
    // 移动构造和赋值
    unique_fd(unique_fd&& other) noexcept : fd_(other.release()) {}
    
    unique_fd& operator=(unique_fd&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.release();
        }
        return *this;
    }
    
    ~unique_fd() {
        close();
    }
    
    // 获取文件描述符
    int get() const noexcept { return fd_; }
    
    // 释放所有权
    int release() noexcept {
        int fd = fd_;
        fd_ = -1;
        return fd;
    }
    
    // 关闭文件描述符
    void close() noexcept {
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    
    // 重置文件描述符
    void reset(int fd = -1) noexcept {
        close();
        fd_ = fd;
    }
    
    // 转换为bool
    explicit operator bool() const noexcept {
        return fd_ != -1;
    }

private:
    int fd_;
};

#endif // UNIQUE_FD_H
