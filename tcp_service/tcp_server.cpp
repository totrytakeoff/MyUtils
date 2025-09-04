#include "tcp_server.hpp"
#include "tcp_session.hpp"
#include "IOService_pool.hpp"
#include <iostream>

TCPServer::TCPServer(unsigned short port)
    : io_context_(IOServicePool::GetInstance().GetIOService()),
      acceptor_(io_context_, tcp::endpoint(tcp::v4(), port))
{
    // 初始化但不启动服务器
    // 用户需要显式调用start()方法启动服务器
}

TCPServer::~TCPServer() {
    stop();
}

void TCPServer::start() {
    if (stopped_) {
        std::cerr << "Cannot start a stopped server\n";
        return;
    }
    
    // 开始接受连接
    start_accept();
    
    std::cout << "Server started on port " << acceptor_.local_endpoint().port() << "\n";
}

void TCPServer::stop() {
    // 确保只执行一次停止操作
    if (!stopped_.exchange(true)) {
        std::cout << "Stopping server...\n";
        
        // 关闭接受器
        error_code ec;
        acceptor_.close(ec);
        if (ec) {
            std::cerr << "Acceptor close error: " << ec.message() << "\n";
        }
        
        // 关闭所有活动会话
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (auto& session : sessions_) {
                session->close();
            }
            sessions_.clear();
        }
        
        std::cout << "Server stopped.\n";
    }
}

void TCPServer::set_connection_handler(std::function<void(TCPSession::Ptr)> callback) {
    connection_handler_ = std::move(callback);
}

void TCPServer::start_accept() {
    // 异步接受新连接
    acceptor_.async_accept(
        [this](error_code ec, tcp::socket socket) {
            handle_accept(ec, std::move(socket));
        });
}

void TCPServer::handle_accept(const error_code& ec, tcp::socket socket) {
    if (ec) {
        // 忽略操作取消的错误（正常关闭时发生）
        if (ec != net::error::operation_aborted) {
            std::cerr << "Accept error: " << ec.message() << "\n";
        }
        return;
    }
    
    try {
        // 创建新会话，使用IOServicePool中的一个io_context
        // 这样可以实现连接的负载均衡
        auto& session_io_context = IOServicePool::GetInstance().GetIOService();
            
        auto session = std::make_shared<TCPSession>(std::move(socket));
        
        // 添加到会话集合，使用互斥锁保护
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.insert(session);
        }
        
        // 设置会话关闭时的清理回调
        session->set_close_callback([this, session] {
            remove_session(session);
        });
        
        // 调用新连接回调
        if (connection_handler_) {
            connection_handler_(session);
        }
        
        // 启动会话
        session->start();
    } catch (const std::exception& e) {
        std::cerr << "Session creation error: " << e.what() << "\n";
    }
    
    // 继续接受新连接
    start_accept();
}

void TCPServer::remove_session(TCPSession::Ptr session) {
    // 从会话集合中移除，使用互斥锁保护
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session);
    if (it != sessions_.end()) {
        sessions_.erase(it);
        std::cout << "Session removed: " << session->remote_endpoint() 
                  << " (" << sessions_.size() << " active sessions)\n";
    }
}