#include "IOService_pool.hpp"
#include <iostream>

IOServicePool::IOServicePool(std::size_t pool_size)
    : next_io_service_(0),
      pool_size_(pool_size)
{
    // 创建指定数量的io_service实例
    for (std::size_t i = 0; i < pool_size_; ++i) {
        IOServicePtr io_service(new IOService());
        io_services_.push_back(io_service);
        
        // 为每个io_service创建work守卫，防止io_service::run()在没有任务时退出
        WorkPtr work(new Work(io_service->get_executor()));
        works_.push_back(std::move(work));
        
        // 为每个io_service创建一个线程
        threads_.emplace_back([io_service] {
            try {
                io_service->run();
            } catch (const std::exception& e) {
                std::cerr << "IOService exception: " << e.what() << std::endl;
            }
        });
    }
}

IOServicePool::~IOServicePool() {
    Stop();
}

boost::asio::io_context& IOServicePool::GetIOService() {
    // 使用轮询方式分配io_service，实现负载均衡
    boost::asio::io_context& io_service = *io_services_[next_io_service_];
    
    // 循环使用io_service
    ++next_io_service_;
    if (next_io_service_ >= io_services_.size()) {
        next_io_service_ = 0;
    }
    
    return io_service;
}

void IOServicePool::Stop() {
    // 停止所有io_service
    for (auto& work : works_) {
        work.reset(); // 移除work守卫，允许io_service在任务完成后退出
    }
    
    // 等待所有线程结束
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // 清空容器
    works_.clear();
    io_services_.clear();
    threads_.clear();
}