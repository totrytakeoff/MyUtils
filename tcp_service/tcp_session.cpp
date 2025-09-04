#include "tcp_session.hpp"
#include <iostream>

using namespace std::chrono_literals;

TCPSession::TCPSession(tcp::socket socket)
    : socket_(std::move(socket)),
      heartbeat_timer_(socket_.get_executor()),
      read_timeout_timer_(socket_.get_executor()) 
{
    // 初始化定时器为永不超时状态
    heartbeat_timer_.expires_at(std::chrono::steady_clock::time_point::max());
    read_timeout_timer_.expires_at(std::chrono::steady_clock::time_point::max());
}

void TCPSession::start() {
    try {
        // 打印连接信息
        std::cout << "New connection from: " << remote_endpoint() << "\n";
        
        // 禁用Nagle算法，减少延迟
        socket_.set_option(tcp::no_delay(true));
        
        // 启动心跳机制
        start_heartbeat();
        
        // 开始读取消息头
        do_read_header();
    } catch (const std::exception& e) {
        std::cerr << "Session start error: " << e.what() << "\n";
        close();
    }
}

void TCPSession::send(const std::string& message) {
    // 将发送操作投递到套接字的执行器中，确保线程安全
    net::post(socket_.get_executor(),
        [this, self = shared_from_this(), msg = std::move(message)]() mutable {
            bool write_in_progress = !write_queue_.empty();
            write_queue_.push_back(std::move(msg));
            
            // 如果没有正在进行的写操作，立即开始发送
            if (!write_in_progress) {
                do_write();
            }
        });
}

void TCPSession::close() {
    if (socket_.is_open()) {
        error_code ec;
        
        // 优雅关闭连接
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        
        // 取消所有定时器
        heartbeat_timer_.cancel();
        read_timeout_timer_.cancel();
        
        // 调用关闭回调
        if (close_callback_) {
            close_callback_();
        }
    }
}

tcp::endpoint TCPSession::remote_endpoint() const {
    return socket_.remote_endpoint();
}

void TCPSession::set_close_callback(std::function<void()> callback) {
    close_callback_ = std::move(callback);
}

void TCPSession::set_message_handler(std::function<void(const std::string&)> callback) {
    message_handler_ = std::move(callback);
}

void TCPSession::start_heartbeat() {
    // 设置心跳定时器
    heartbeat_timer_.expires_after(heartbeat_interval);
    heartbeat_timer_.async_wait(
        [this, self = shared_from_this()](error_code ec) {
            if (ec) return; // 定时器被取消
            
            if (!socket_.is_open()) return;
            
            // 发送心跳消息
            send("HEARTBEAT");
            
            // 设置下一次心跳
            start_heartbeat();
        });
}

void TCPSession::reset_read_timeout() {
    // 设置读超时定时器
    read_timeout_timer_.expires_after(read_timeout);
    read_timeout_timer_.async_wait(
        [this, self = shared_from_this()](error_code ec) {
            if (ec) return; // 定时器被取消
            
            // 读超时，关闭连接
            std::cout << "Connection timeout: " << remote_endpoint() << "\n";
            close();
        });
}

void TCPSession::do_read_header() {
    // 重置读超时定时器
    reset_read_timeout();
    
    auto self(shared_from_this());
    
    // 异步读取4字节的消息头（消息长度）
    net::async_read(socket_, net::buffer(header_),
        [this, self](error_code ec, size_t /*bytes*/) {
            // 取消读超时定时器
            read_timeout_timer_.cancel();
            
            if (ec) {
                handle_error(ec);
                return;
            }
            
            // 将网络字节序转换为主机字节序
            uint32_t body_length = ntohl(*reinterpret_cast<uint32_t*>(header_.data()));
            
            // 检查消息长度是否合法
            if (body_length > max_body_length) {
                std::cerr << "Message too large: " << body_length 
                          << " bytes from " << remote_endpoint() << "\n";
                close();
                return;
            }
            
            // 继续读取消息体
            do_read_body(body_length);
        });
}

void TCPSession::do_read_body(uint32_t length) {
    auto self(shared_from_this());
    
    // 调整缓冲区大小以容纳消息体
    body_buffer_.resize(length);
    
    // 异步读取消息体
    net::async_read(socket_, net::buffer(body_buffer_),
        [this, self, length](error_code ec, size_t /*bytes*/) {
            if (ec) {
                handle_error(ec);
                return;
            }
            
            // 构造完整消息
            std::string message(body_buffer_.data(), length);
            
            // 调用消息处理回调
            if (message_handler_) {
                message_handler_(message);
            }
            
            // 继续读取下一条消息
            do_read_header();
        });
}

void TCPSession::do_write() {
    auto self(shared_from_this());
    
    // 获取队列中的第一条消息
    const std::string& message = write_queue_.front();
    
    // 构造带长度前缀的消息
    uint32_t length = htonl(static_cast<uint32_t>(message.size()));
    std::vector<net::const_buffer> buffers;
    buffers.push_back(net::buffer(&length, sizeof(length)));
    buffers.push_back(net::buffer(message));
    
    // 异步发送消息
    net::async_write(socket_, buffers,
        [this, self](error_code ec, size_t /*bytes*/) {
            if (ec) {
                handle_error(ec);
                return;
            }
            
            // 从队列中移除已发送的消息
            write_queue_.pop_front();
            
            // 如果队列中还有消息，继续发送
            if (!write_queue_.empty()) {
                do_write();
            }
        });
}

void TCPSession::handle_error(const error_code& ec) {
    // 忽略正常关闭的错误
    if (ec == net::error::eof || 
        ec == net::error::connection_reset ||
        ec == net::error::operation_aborted) {
        std::cout << "Connection closed: " << remote_endpoint() << "\n";
    } 
    // 处理其他错误
    else if (ec) {
        std::cerr << "Connection error [" << remote_endpoint() 
                  << "]: " << ec.message() << "\n";
    }
    
    // 关闭连接
    close();
}