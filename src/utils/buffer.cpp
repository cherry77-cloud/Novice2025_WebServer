#include "buffer.h"
#include <cerrno>

Buffer::Buffer(int initBuffersize) : buffer_(initBuffersize), readPos_(0), writePos_(0) {}

// 移动构造函数
Buffer::Buffer(Buffer&& other) noexcept 
    : buffer_(std::move(other.buffer_)), 
      readPos_(other.readPos_), 
      writePos_(other.writePos_) {
    other.readPos_ = 0;
    other.writePos_ = 0;
}

// 移动赋值操作符
Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        buffer_ = std::move(other.buffer_);
        readPos_ = other.readPos_;
        writePos_ = other.writePos_;
        other.readPos_ = 0;
        other.writePos_ = 0;
    }
    return *this;
}

size_t Buffer::readableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::writeableBytes() const {
    return buffer_.size() - writePos_;
}

const char* Buffer::curReadPtr() const {
    return BeginPtr_() + readPos_;
}

const char* Buffer::curWritePtrConst() const {
    return BeginPtr_() + writePos_;
}

char* Buffer::curWritePtr() {
    return BeginPtr_() + writePos_;
}

void Buffer::updateReadPtr(size_t len) {
    assert(len <= readableBytes());
    readPos_ += len;
}

void Buffer::updateReadPtrUntilEnd(const char* end) {
    assert(end >= curReadPtr());
    updateReadPtr(end - curReadPtr());
}

void Buffer::updateWritePtr(size_t len) {
    assert(len <= writeableBytes());
    writePos_ += len;
}

void Buffer::initPtr() {
    readPos_ = 0;
    writePos_ = 0;
}

void Buffer::allocateSpace(size_t len) {
    if (writeableBytes() + readPos_ < len) {
        buffer_.resize(writePos_ + len + 1);
    } else {
        size_t readable = readableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readable;
        assert(readable == readableBytes());
    }
}

void Buffer::ensureWriteable(size_t len) {
    if (writeableBytes() < len) {
        allocateSpace(len);
    }
    assert(writeableBytes() >= len);
}

void Buffer::append(const char* str, size_t len) {
    assert(str);
    ensureWriteable(len);
    std::copy(str, str + len, curWritePtr());
    updateWritePtr(len);
}

void Buffer::append(const char* str) {
    assert(str);
    append(str, strlen(str));
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

void Buffer::append(std::string_view str) {
    append(str.data(), str.size());
}

void Buffer::append(const void* data, size_t len) {
    assert(data);
    append(static_cast<const char*>(data), len);
}

void Buffer::append(Buffer&& buffer) {
    if (buffer.readableBytes() == 0) {
        return;  // 空buffer，直接返回
    }
    
    // 如果当前buffer为空，直接swap整个buffer
    if (readableBytes() == 0) {
        std::swap(buffer_, buffer.buffer_);
        readPos_ = buffer.readPos_;
        writePos_ = buffer.writePos_;
        buffer.initPtr();
        return;
    }
    
    // 否则追加数据
    append(buffer.curReadPtr(), buffer.readableBytes());
    buffer.initPtr();  // 清空移动源
}

ssize_t Buffer::readFd(int fd, int* Errno) {
    // 使用thread_local缓冲区避免重复栈分配
    thread_local static char buff[65536];
    struct iovec iov[2];
    const size_t writable = writeableBytes();

    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno = errno;
    } else if (static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    } else {
        writePos_ = buffer_.size();
        append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::writeFd(int fd, int* Errno) {
    size_t readSize = readableBytes();
    ssize_t len = write(fd, curReadPtr(), readSize);
    if (len < 0) {
        *Errno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}

std::string Buffer::AlltoStr() {
    std::string str(curReadPtr(), readableBytes());
    initPtr();
    return str;
}

std::string_view Buffer::view() const {
    return std::string_view(curReadPtr(), readableBytes());
}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::BeginPtr_() const {
    return &*buffer_.begin();
}