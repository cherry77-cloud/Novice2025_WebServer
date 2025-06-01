#ifndef DATE_CACHE_H
#define DATE_CACHE_H

#include <atomic>
#include <chrono>
#include <ctime>
#include <string>
#include <thread>

// 全局Date头缓存，每秒更新一次
class DateCache {
public:
    static DateCache& getInstance() {
        static DateCache instance;
        return instance;
    }
    
    // 获取缓存的Date字符串
    const char* getDateString() const {
        return cached_date_.load(std::memory_order_acquire);
    }
    
    // 启动后台更新线程
    void start() {
        if (!running_.exchange(true)) {
            updateThread_ = std::thread([this]() {
                while (running_.load(std::memory_order_acquire)) {
                    updateDateString();
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            });
            // 保持线程joinable，不要detach，确保析构时能正确join
        }
    }
    
    // 停止后台更新
    void stop() {
        running_.store(false, std::memory_order_release);
        // 等待线程安全退出，避免内存泄漏
        if (updateThread_.joinable()) {
            updateThread_.join();
        }
    }

private:
    DateCache() {
        updateDateString(); // 初始化
    }
    
    ~DateCache() {
        stop();
        delete[] buffer1_;
        delete[] buffer2_;
    }
    
    void updateDateString() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto* tm = std::gmtime(&time_t); // GMT时间
        
        // 双缓冲机制避免竞争
        char* current_buffer = use_buffer1_ ? buffer1_ : buffer2_;
        
        // 格式：Date: Wed, 22 Jul 2009 19:15:56 GMT\r\n
        std::strftime(current_buffer, 64, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", tm);
        
        // 原子更新指针
        cached_date_.store(current_buffer, std::memory_order_release);
        use_buffer1_ = !use_buffer1_;
    }
    
    char* buffer1_ = new char[64];  // 双缓冲
    char* buffer2_ = new char[64];
    std::atomic<bool> use_buffer1_{true};
    std::atomic<const char*> cached_date_;
    std::atomic<bool> running_{false};
    std::thread updateThread_;
};

// 全局访问函数
inline const char* getCachedDateHeader() {
    return DateCache::getInstance().getDateString();
}

inline void startDateCache() {
    DateCache::getInstance().start();
}

inline void stopDateCache() {
    DateCache::getInstance().stop();
}

#endif // DATE_CACHE_H 