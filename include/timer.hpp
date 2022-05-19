#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <ctime>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <assert.h>

typedef std::chrono::high_resolution_clock CLOCK;
typedef CLOCK::time_point TimeStamp;
typedef std::chrono::milliseconds MS;
typedef std::function<void()> TimeoutCallBack;

// 定时器节点
class TimerNode {
public:
    int id;  // 每个socket连接对应一个定时器
    TimeStamp timeExpire;    // 过期时间
    TimeoutCallBack callbackFunc;   // 回调函数用于删除定时器时关闭对应的HTTP连接

    // 重载比较运算符<
    bool operator< (const TimerNode& t) {
        return timeExpire < t.timeExpire;
    }
};

// 管理定时器
class TimerManager {
public:
    TimerManager() { heapTimer_.reserve(128); };  // reserve()只扩充capability，不改变size
    ~TimerManager() { clear();};

    void addTimer(int id, int timewait, const TimeoutCallBack& cbfunc); // 添加定时器
    void handleExpiredTimer();  // 处理超时的连接
    int getNextHandle();    // 下一次处理超时连接的时间

    void update(size_t id, int timewait);  // 更新指定id的定时时长并调整节点
    void work(size_t id);  // 删除指定id的定时器，触发回调函数

    void pop(); // 弹出顶端到期的定时器节点
    void clear();   // 清除堆和哈希表

private:
    // 在vector基础上实现堆结构
    void delTimer(size_t i);    // 删除指定的定时器
    void siftup(size_t i);     // 比较父子节点的定时，i小,往上调
    void siftdown(size_t i);   // 比较父子节点的定时，i大,往下调
    void swapNode(size_t i, size_t j);  // 交换定时器节点

    std::vector<TimerNode> heapTimer_;   // vector存储定时器，模拟堆
    std::unordered_map<size_t, size_t> ref_; // 哈希表：key = id, value = index of Timer in heapTimer_
    // 补充：ref_用于映射一个client fd对应的TimerNode在堆中的索引
};

#endif