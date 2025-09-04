#pragma once

#include "tcp_session.hpp"
#include <boost/asio.hpp>
#include <set>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>

/**
 * @class TCPServer
 * @brief TCP服务器类，管理所有TCP连接
 * 
 * 功能包括：
 * 1. 接受新的TCP连接
 * 2. 管理所有活动会话
 * 3. 多线程支持
 * 4. 优雅关闭机制
 */
class TCPServer {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     */
    TCPServer(unsigned short port);
    
    /**
     * @brief 析构函数，自动停止服务器
     */
    ~TCPServer();
    
    /**
     * @brief 停止服务器
     */
    void stop();
    
    /**
     * @brief 设置新连接建立时的回调函数
     * @param callback 回调函数，参数为TCPSession::Ptr
     */
    void set_connection_handler(std::function<void(TCPSession::Ptr)> callback);

private:
    // 开始接受新连接
    void start_accept();
    
    // 处理新连接
    void handle_accept(const error_code& ec, tcp::socket socket);
    
    // 移除会话
    void remove_session(TCPSession::Ptr session);

    net::io_context& io_context_;                // I/O上下文引用（来自IOServicePool）
    tcp::acceptor acceptor_;                      // 连接接受器
    std::mutex sessions_mutex_;                   // 会话集合互斥锁
    std::set<TCPSession::Ptr> sessions_;          // 活动会话集合
    std::atomic<bool> stopped_{false};            // 服务器停止标志
    
    std::function<void(TCPSession::Ptr)> connection_handler_; // 新连接回调
};