#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/******************************************************************************
 *
 * @file       thread_pool.h
 * @brief      线程池类
 *
 * @author     myself
 * @date       2025/7/12
 * @history    
 *****************************************************************************/

#include "singleton.h"
#include "logger.h"
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool : public Singleton<ThreadPool> {
    friend class Singleton<ThreadPool>;
public:
    ~ThreadPool();
    
    void Initialize(size_t threadCount = std::thread::hardware_concurrency());
    void Shutdown();
    
    /**
     * @brief 提交任务到线程池
     * 
     * 这是一个可变参数模板函数，可以接受任意类型的函数和参数
     * 
     * @tparam F 函数类型（可以是函数指针、函数对象、lambda等）
     * @tparam Args 参数包，表示任意数量的参数类型
     * @param f 要执行的函数（使用完美转发）
     * @param args 函数参数（使用完美转发）
     * @return std::future<return_type> 用于获取任务执行结果的future对象
     * 
     * 高级语法说明：
     * 1. template<class F, class... Args> - 可变参数模板
     * 2. F&& f, Args&&... args - 完美转发，保持参数的原始值类型
     * 3. auto -> std::future<...> - 尾置返回类型
     * 4. std::result_of<F(Args...)>::type - 类型萃取，获取函数返回类型
     */
    template<class F, class... Args>
    auto Enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    size_t GetThreadCount() const { return _threads.size(); }
    size_t GetTaskCount() const;
    bool IsShutdown() const { return _stop; }

private:
    ThreadPool();
    
    void WorkerThread();
    
    std::vector<std::thread> _threads;
    std::queue<std::function<void()>> _tasks;
    mutable std::mutex _queueMutex;
    std::condition_variable _condition;
    std::atomic<bool> _stop;
    std::atomic<size_t> _taskCount;
};

/**
 * @brief Enqueue 方法的实现（异步）
 * 
 * 这个函数展示了现代 C++ 的多个高级特性：
 * 1. 可变参数模板 (Variadic Templates)
 * 2. 完美转发 (Perfect Forwarding)
 * 3. 类型萃取 (Type Traits)
 * 4. 智能指针和 RAII
 * 5. Lambda 表达式
 * 6. 异常安全
 */
template<class F, class... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    // 使用类型萃取获取函数的返回类型
    // std::result_of<F(Args...)>::type 会在编译时推导出函数调用的返回类型
    // 例如：如果 F 是 int(int, double)，Args 是 int, double
    // 那么 return_type 就是 int
    using return_type = typename std::result_of<F(Args...)>::type;
    
    // 创建 packaged_task 来包装函数调用
    // std::packaged_task<return_type()> 表示一个无参数、返回 return_type 的函数对象
    // std::bind 将函数 f 和参数 args 绑定成一个无参数的函数对象
    // std::forward 进行完美转发，保持参数的原始值类型（左值/右值）
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    // 获取与 packaged_task 关联的 future 对象
    // 调用者可以通过这个 future 获取任务执行结果
    std::future<return_type> result = task->get_future();
    
    {
        // RAII 锁管理：作用域结束自动解锁
        std::lock_guard<std::mutex> lock(_queueMutex);
        
        // 检查线程池是否已关闭
        if (_stop) {
            throw std::runtime_error("ThreadPool is shutdown");
        }
        
        // 将任务添加到队列
        // [task]() { (*task)(); } 是一个 lambda 表达式
        // [task] 表示值捕获，将 task（shared_ptr）复制到 lambda 中
        // (*task)() 调用 packaged_task 的 operator()
        _tasks.emplace([task]() { (*task)(); });
        ++_taskCount;
    }
    
    // 通知一个等待的工作线程有新任务
    _condition.notify_one();
    return result;
}

#endif // THREAD_POOL_H 