#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <ctime>
#include <deque>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>

#include "http_connection.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

class TimerNode {
public:
    int id;
    TimeStamp expire;
    TimeoutCallBack cb;

    // 注意：operator< 为正序，配合heap算法实现最小堆（expire越小越优先）
    bool operator<(const TimerNode& t) {
        return expire < t.expire;
    }
};

class TimerManager {
public:
    TimerManager() {
        heap_.reserve(64);
    }
    ~TimerManager() {
        clear();
    }
    void addTimer(int id, int timeout, const TimeoutCallBack& cb);
    void handle_expired_event();
    int getNextHandle();

    void update(int id, int timeout);
    void work(int id);

    void pop();
    void clear();

private:
    void del_(size_t i);
    void siftup_(size_t i);
    bool siftdown_(size_t index, size_t n);
    void swapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;
};

#endif  // TIMER_H