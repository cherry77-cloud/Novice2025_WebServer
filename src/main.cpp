#include <unistd.h>
#include <thread>
#include <sys/resource.h>
#include <iostream>
#include "webserver.h"

void optimizeSystem() {
    // 设置进程优先级
    if (setpriority(PRIO_PROCESS, 0, -5) == 0) {
        std::cout << "Set process priority to high" << std::endl;
    }
    
    // 设置文件描述符限制
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
        std::cout << "Current fd limit: " << rlim.rlim_cur << std::endl;
        if (rlim.rlim_cur < 100000) {
            rlim.rlim_cur = std::min(static_cast<rlim_t>(100000), rlim.rlim_max);
            if (setrlimit(RLIMIT_NOFILE, &rlim) == 0) {
                std::cout << "Updated fd limit to: " << rlim.rlim_cur << std::endl;
            }
        }
    }
    
    // 设置栈大小限制 - 增加到16MB
    if (getrlimit(RLIMIT_STACK, &rlim) == 0) {
        if (rlim.rlim_cur < 16 * 1024 * 1024) {
            rlim.rlim_cur = 16 * 1024 * 1024; // 16MB
            if (setrlimit(RLIMIT_STACK, &rlim) == 0) {
                std::cout << "Updated stack limit to: " << rlim.rlim_cur << " bytes" << std::endl;
            }
        }
    }
    
    // 设置虚拟内存限制
    if (getrlimit(RLIMIT_AS, &rlim) == 0) {
        std::cout << "Virtual memory limit: " << rlim.rlim_cur << std::endl;
    }
}

int main() 
{
    // 系统优化
    optimizeSystem();
    
    // 使用更保守的线程数策略：CPU核心数的1.5倍，最少4个，最多16个
    size_t thread_num = std::max(4u, std::min(16u, 
        static_cast<unsigned int>(std::thread::hardware_concurrency() * 1.5)));
    std::cout << "Using " << thread_num << " worker threads" << std::endl;
    
    // 端口8000，边缘触发模式，60秒超时，不启用linger，使用优化的线程数
    WebServer server(8000, 3, 60000, false, thread_num);
    server.Start();
    
    return 0;
}