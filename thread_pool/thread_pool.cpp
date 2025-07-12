#include "thread_pool.h"
#include <algorithm>

ThreadPool::ThreadPool() : _stop(false), _taskCount(0) {
}

ThreadPool::~ThreadPool() {
    Shutdown();
}

void ThreadPool::Initialize(size_t threadCount) {
    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
    }
    
    _stop = false;
    _taskCount = 0;
    
    // 创建工作线程
    for (size_t i = 0; i < threadCount; ++i) {
        _threads.emplace_back(&ThreadPool::WorkerThread, this);
    }
    
    LOG_INFO("ThreadPool initialized with {} threads", threadCount);
}

void ThreadPool::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _stop = true;
    }
    
    _condition.notify_all();
    
    // 等待所有线程完成
    for (auto& thread : _threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    _threads.clear();
    
    LOG_INFO("ThreadPool shutdown completed");
}

void ThreadPool::WorkerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            
            _condition.wait(lock, [this] {
                return _stop || !_tasks.empty();
            });
            
            if (_stop && _tasks.empty()) {
                return;
            }
            
            if (!_tasks.empty()) {
                task = std::move(_tasks.front());
                _tasks.pop();
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in worker thread: {}", e.what());
            }
            
            --_taskCount;
        }
    }
}

size_t ThreadPool::GetTaskCount() const {
    return _taskCount.load();
} 