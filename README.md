# 🚀 高性能C++ Web服务器

一个基于C++17实现的高性能Web服务器，采用Reactor模式和线程池设计，支持静态资源服务和CGI动态内容处理。

## ✨ 项目特性

### 🎯 核心特性
- **⚡ 高并发处理**：基于Epoll的I/O多路复用，支持ET/LT模式
- **🔥 零拷贝优化**：使用mmap内存映射和sendfile系统调用
- **🔒 无锁队列**：基于MPMCQueue的高性能线程池实现
- **💾 智能缓存**：HTTP Date头缓存，文件类型缓存
- **🐍 CGI支持**：完整的CGI/1.1协议实现，支持Python脚本
- **🔗 Keep-Alive**：持久连接支持，减少连接开销
- **⏰ 定时器管理**：基于最小堆的高效定时器

### 🏎️ 性能优化
- 🚄 TCP_NODELAY和TCP_CORK优化
- 🔄 SO_REUSEADDR和SO_REUSEPORT支持
- 🎯 CPU亲和性绑定
- 📦 预分配缓冲区
- 🛡️ RAII资源管理

## 🏗️ 项目目录
```text
├── 📂 src/
│   ├── 📄 main.cpp              # 程序入口
│   ├── 🛠️ CMakeLists.txt        # CMake构建配置
│   ├── 📂 core/                 # 核心组件
│   │   ├── 🌐 webserver.cpp     # Web服务器主类实现
│   │   ├── 🌐 webserver.h       # Web服务器主类头文件
│   │   ├── 📡 epoll.cpp         # Epoll封装实现（文件名是epoll.cpp）
│   │   ├── 📡 epoller.h         # Epoll封装头文件
│   │   └── 👷 threadpool.h      # 无锁线程池
│   ├── 📂 http/                 # HTTP处理
│   │   ├── 🔌 http_connection.cpp   # HTTP连接管理实现
│   │   ├── 🔌 http_connection.h     # HTTP连接管理头文件
│   │   ├── 📨 http_request.cpp      # HTTP请求解析实现
│   │   ├── 📨 http_request.h        # HTTP请求解析头文件
│   │   ├── 📤 http_response.cpp     # HTTP响应生成实现
│   │   ├── 📤 http_response.h       # HTTP响应生成头文件
│   │   ├── 🐍 cgi_handler.cpp       # CGI处理器实现
│   │   └── 🐍 cgi_handler.h         # CGI处理器头文件
│   └── 📂 utils/                # 工具类
│       ├── 💾 buffer.cpp        # 高性能缓冲区实现
│       ├── 💾 buffer.h          # 高性能缓冲区头文件
│       ├── ⏰ timer.cpp         # 定时器管理实现
│       ├── ⏰ timer.h           # 定时器管理头文件
│       ├── 📅 date_cache.h      # Date头缓存
│       └── 🔒 unique_fd.h       # RAII文件描述符
```

## 🔧 编译和运行

### 📋 环境要求
- 🐧 Linux系统（支持epoll）
- 🛠️ C++17编译器（GCC 7+或Clang 5+）
- 📦 CMake 3.10+
- 🐍 Python3（用于CGI脚本）

### 🏗️ 编译步骤

```bash
# 📁 创建构建目录
mkdir build && cd build

# ⚙️ 配置CMake（Release模式）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 🔨 编译
make -j$(nproc)

# 🚀 运行服务器
./bin/webserver
```


## 🔬 技术细节
### 🎯 Reactor模式实现

- 🎮 主线程负责accept新连接和epoll事件分发
- 👷 工作线程池处理读写事件和业务逻辑
- ⚡ 支持ET/LT两种触发模式

### 🔒 无锁队列设计

- 💡 基于Folly MPMCQueue的思想
- ⚛️ 使用原子操作和内存序保证线程安全
- 🎯 避免false sharing的缓存行对齐

### 💾 内存管理优化

- 📦 预分配缓冲区减少内存分配
- ♻️ 使用对象池复用连接对象
- 🛡️ RAII自动管理资源生命周期

### 📨 HTTP解析优化
- 🤖 状态机解析HTTP请求
- 🚀 string_view避免不必要的字符串拷贝
