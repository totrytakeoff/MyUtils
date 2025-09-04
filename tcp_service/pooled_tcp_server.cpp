// pooled_tcp_server.cpp
#include "pooled_tcp_server.hpp"
#include <iostream>
#include <boost/asio/post.hpp>

PooledTCPServer::PooledTCPServer(unsigned short port, bool own_thread_pool, size_t thread_count)
    : own_thread_pool_(own_thread_pool),
      io_context_(own_thread_pool 
                  ? IOServicePool::instance().Initialize(thread_count).GetIOService()
                  : IOServicePool::instance().GetIOService()),
      acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)),
      own_thread_pool_(own_thread_pool)
{
    // 如果使用独立线程池，确保已正确初始化
    if (own_thread_pool && thread_count > 0) {
        // 触发线程池初始化
        boost::asio::post(io_context_, []() {});
    }
}

void PooledTCPServer::start() {
    if (stopped_) return;
    
    std::cout << "Server running on port " << acceptor_.local_endpoint().port() << "\n"
              << "Using IOServicePool with thread count: " 
              << io_context_.get_executor().context().get_pool_size() << std::endl;
    
    // 启动多个异步accept操作提高吞吐量
    for (int i = 0; i < 4; ++i) {
        do_accept();
    }
}

void PooledTCPServer::stop() {
    if (stopped_.exchange(true)) return;
    
    boost::system::error_code ec;
    acceptor_.close(ec);
    
    // 清理所有会话
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& session : sessions_) {
            session->close();
        }
        sessions_.clear();
    }

    // 如果拥有独立线程池，触发停止
    if (own_thread_pool_) {
        IOServicePool::instance().Stop();
    }
}

void PooledTCPServer::on_connection(std::function<void(tcp::socket)> cb) {
    connection_handler_ = [this, cb](tcp::socket socket) {
        std::shared_ptr<tcp_session> session;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            session = std::make_shared<tcp_session>(std::move(socket));
            sessions_.insert(session);
        }

        session->set_close_callback([this, session]() {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.erase(session);
        });

        session->start();
    };
}

void PooledTCPServer::do_accept() {
    if (stopped_) return;
    
    acceptor_.async_accept(
        [this](error_code ec, tcp::socket socket) {
            if (ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    // 正常关闭
                    return;
                }
                std::cerr << "[ERROR] Accept failed: " << ec.message() << std::endl;
                return;
            }

            if (connection_handler_) {
                connection_handler_(std::move(socket));
            }
            
            do_accept();
        });
}