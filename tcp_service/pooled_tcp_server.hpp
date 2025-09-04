// pooled_tcp_server.hpp
#pragma once

#include <boost/asio.hpp>
#include "tcp_session.hpp"
#include "IOService_pool.hpp"
#include <mutex>

using namespace boost::asio;
using ip::tcp;
using error_code = boost::system::error_code;

/**
 * @class PooledTCPServer
 * @brief 基于IOServicePool的TCP服务器实现
 * 使用全局IOService池处理异步IO操作，实现高效的连接管理
 */
class PooledTCPServer {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param own_thread_pool 是否独占线程池
     * @param thread_count 线程池大小（own_thread_pool为true时生效）
     */
    PooledTCPServer(unsigned short port, bool own_thread_pool = false, size_t thread_count = 0);
    
    /**
     * @brief 启动服务器
     * 开始监听并接受连接
     */
    void start();
    
    /**
     * @brief 停止服务器
     * 关闭所有连接并释放资源
     */
    void stop();
    
    /**
     * @brief 设置新连接回调
     * @param cb 回调函数
     */
    void on_connection(std::function<void(tcp::socket)> cb);

private:
    void do_accept();
    
    io_context& io_context_;          // 引用IOServicePool的上下文
    tcp::acceptor acceptor_;          // 连接接收器
    std::function<void(tcp::socket)> connection_handler_;
    bool stopped_ = false;
    const bool own_thread_pool_;      // 是否拥有独立线程池
    std::mutex sessions_mutex_;       // 会话集合互斥锁
    std::set<tcp_session::Ptr> sessions_; // 活动会话集合
};