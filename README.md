# 高性能C++ Web服务器

一个基于C++17实现的高性能Web服务器，采用Reactor模式和线程池设计，支持静态资源服务和CGI动态内容处理。

## 项目特性

### 核心特性
- **高并发处理**：基于Epoll的I/O多路复用，支持ET/LT模式
- **零拷贝优化**：使用mmap内存映射和sendfile系统调用
- **无锁队列**：基于MPMCQueue的高性能线程池实现
- **智能缓存**：HTTP Date头缓存，文件类型缓存
- **CGI支持**：完整的CGI/1.1协议实现，支持Python脚本
- **Keep-Alive**：持久连接支持，减少连接开销
- **定时器管理**：基于最小堆的高效定时器

### 性能优化
- TCP_NODELAY和TCP_CORK优化
- SO_REUSEADDR和SO_REUSEPORT支持
- CPU亲和性绑定
- 预分配缓冲区
- RAII资源管理
