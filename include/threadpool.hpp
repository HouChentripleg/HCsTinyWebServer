#ifndef MY_THREAD_POOL_H
#define MY_THREAD_POOL_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
#include <stdexcept>

class ThreadPool {
public:
    ThreadPool(size_t threadNum) : isStop(false) {
    for(size_t i = 0; i < threadNum; i++) {
        // 向线程池中加入新开辟的线程
        WorkThreads.emplace_back([this] {
            while(true) {
                std::function<void()> task;  // 创建一个封装void()函数的function类型的对象task，用于后续接收从任务队列中弹出的真实任务
                {
                    std::unique_lock<std::mutex> lock(this->m_mutex);
                    // 当isStop==false&&tasks.empty()==1时当前线程阻塞，直至m_cond被通知
                    this->m_cond.wait(lock, [this] {
                        return this->isStop || !this->tasks.empty();});
                    // 当isStop==true&&tasks.empty()==1时跳出死循环
                    if(this->isStop && this->tasks.empty()) {
                        return;
                    }
                    // 弹出工作队列中的最旧的任务
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                // 执行任务
                task();
            }
        });
    }
}

    // 添加任务到工作队列
    template <typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type> {
    // 函数F(Args...)的返回类型，future<>用于访问异步操作的结果
    using returnType = typename std::result_of<F(Args...)>::type;
    // 创建智能指针task，指向一个用(bind())初始化的packaged_task<returnType()>类的对象
    // 等同于packaged_task<returnType()> t(bind(forward<F>(f), forward<Args>(args)...))，将task指向t
    // packaged_task<returnType()>包装了一个返回值为returnType的函数
    // bind(f,a,b)捆绑了一个f(a,b)的函数
    // forward完整保留参数的引用类型(左/右值引用)进行转发
    auto task = std::make_shared<std::packaged_task<returnType()>> (
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    // task异步执行完毕后将结果保存在res中
    std::future<returnType> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if(isStop) {
            throw std::runtime_error("threadpool already stopped, enqueue failed");
        }
        // 将task指向的f(args)插入工作队列中
        // (*task)()即为f(args)
        tasks.emplace([task]{(*task)();});
    }
    // 加入任务后，唤醒一个线程
    m_cond.notify_one();
    // 线程执行完毕后，将异步执行结果返回
    return res;
}
    
    ~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        isStop = true;
    }
    // 条件变量唤醒所有线程，所有线程从37-38行向下执行
    m_cond.notify_all();
    // isStop==true&&tasks.empty()==1时，线程跳出while(true)循环
    for(std::thread &worker : WorkThreads) {
        // 每个线程都退出后，主线程再退出
        worker.join();
    }
}
private:
    std::vector<std::thread> WorkThreads;   // 线程池
    std::queue<std::function<void()>> tasks;  // 工作队列
    std::mutex m_mutex;  // 工作队列的互斥锁
    std::condition_variable m_cond;  // 工作队列的条件变量
    bool isStop;    // 线程池是否关闭
};

#endif
