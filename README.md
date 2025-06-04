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
.
├── 📂 src/
│   ├── 📄 main.cpp              # 程序入口
│   ├── 🛠️ CMakeLists.txt        # CMake构建配置
│   ├── 📂 core/                 # 核心组件
│   │   ├── 🌐 webserver.cpp/h   # Web服务器主类
│   │   ├── 📡 epoll.cpp/h       # Epoll封装
│   │   └── 👷 threadpool.h      # 无锁线程池
│   ├── 📂 http/                 # HTTP处理
│   │   ├── 🔌 http_connection.cpp/h  # HTTP连接管理
│   │   ├── 📨 http_request.cpp/h     # HTTP请求解析
│   │   ├── 📤 http_response.cpp/h    # HTTP响应生成
│   │   └── 🐍 cgi_handler.cpp/h      # CGI处理器
│   └── 📂 utils/                # 工具类
│       ├── 💾 buffer.cpp/h      # 高性能缓冲区
│       ├── ⏰ timer.cpp/h       # 定时器管理
│       ├── 📅 date_cache.h      # Date头缓存
│       └── 🔒 unique_fd.h       # RAII文件描述符
├── 📂 resources/                # 静态资源目录
│   ├── 🏠 index.html
│   ├── ❌ 400.html
│   ├── 🚫 403.html
│   └── 🔍 404.html
└── 📂 cgi-bin/                  # CGI脚本目录
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
