#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <future>
#include <memory>
#include <chrono>
#include <pthread.h>
#include <sched.h>

// CPU绑核工具函数
inline void bindToCore(size_t coreId) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// 受Folly MPMCQueue启发的高性能无锁队列
template<typename T, size_t Size>
class MPMCQueue {
    static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");
    
    struct alignas(64) Slot {
        std::atomic<size_t> turn{0};
        T data;
    };
    
    alignas(64) Slot slots_[Size];
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    
    static constexpr size_t kMask = Size - 1;

public:
    MPMCQueue() noexcept {
        for (size_t i = 0; i < Size; ++i) {
            slots_[i].turn.store(i, std::memory_order_relaxed);
        }
    }

    template<typename... Args>
    bool enqueue(Args&&... args) noexcept {
        size_t tail = tail_.load(std::memory_order_relaxed);
        
        for (;;) {
            Slot& slot = slots_[tail & kMask];
            size_t turn = slot.turn.load(std::memory_order_acquire);
            
            if (turn == tail) {
                if (tail_.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed)) {
                    slot.data = T(std::forward<Args>(args)...);
                    slot.turn.store(tail + 1, std::memory_order_release);
                    return true;
                }
            } else if (turn < tail) {
                return false; // 队列满
            } else {
                tail = tail_.load(std::memory_order_relaxed);
            }
        }
    }

    bool dequeue(T& item) noexcept {
        size_t head = head_.load(std::memory_order_relaxed);
        
        for (;;) {
            Slot& slot = slots_[head & kMask];
            size_t turn = slot.turn.load(std::memory_order_acquire);
            
            if (turn == head + 1) {
                if (head_.compare_exchange_weak(head, head + 1, std::memory_order_relaxed)) {
                    item = std::move(slot.data);
                    slot.turn.store(head + Size, std::memory_order_release);
                    return true;
                }
            } else if (turn < head + 1) {
                return false; // 队列空
            } else {
                head = head_.load(std::memory_order_relaxed);
            }
        }
    }

    bool empty() const noexcept {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return head == tail;
    }

    size_t size() const noexcept {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return tail - head;
    }
};

class ThreadPool {
private:
    using Task = std::function<void()>;
    static constexpr size_t QUEUE_SIZE = 2048;
    
    MPMCQueue<Task, QUEUE_SIZE> queue_;
    std::vector<std::thread> workers_;
    std::atomic<bool> stop_{false};

public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) {
        if (threads == 0) threads = 1;
        
        const size_t cpuCnt = std::thread::hardware_concurrency();
        workers_.reserve(threads);
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this, i, cpuCnt] {
                // 绑核：worker_i → core_(i % cpuCnt)
                bindToCore(i % cpuCnt);
                
                Task task;
                while (!stop_.load(std::memory_order_acquire)) {
                    if (queue_.dequeue(task)) {
                        task();
                    } else {
                        std::this_thread::yield();
                    }
                }
                
                // 处理剩余任务
                while (queue_.dequeue(task)) {
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        stop_.store(true, std::memory_order_release);
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        
        if (stop_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool is stopping");
        }
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> future = task->get_future();
        
        // 尝试多次提交，受Folly启发的重试策略
        for (int retries = 0; retries < 100; ++retries) {
            if (queue_.enqueue([task]() { (*task)(); })) {
                return future;
            }
            
            if (retries < 10) {
                std::this_thread::yield();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
        
        (*task)();
        return future;
    }

    size_t size() const noexcept {
        return queue_.size();
    }
    
    size_t getWorkerCount() const noexcept {
        return workers_.size();
    }
};

#endif // THREADPOOL_H