#pragma once
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include "../thread_pool/thread_pool.h"

/*!
 * @brief WebSocket客户端类，实现异步双工通信
 * @example
 * 创建客户端：WebSocketClient client(ioc, pool);
 * 设置消息回调：client.set_message_handler(你的处理函数);
 * 连接服务器：client.connect("host", "port", "/path");
 */
namespace websocket {

// 命名空间别名简化代码
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

class WebSocketClient {
public:
    /**
     * @param ioc 异步IO上下文，管理所有异步操作
     * @param pool 线程池，用于执行耗时操作避免阻塞IO
     */
    WebSocketClient(boost::asio::io_context& ioc, ThreadPool& pool);
    
    //! @brief 异步连接服务器
    void connect(const std::string& host, const std::string& port, const std::string& path);
    
    //! @brief 线程安全的消息发送方法
    void send(const std::string& message);
    
    //! @brief 优雅关闭连接
    void close();
    
    //! @param handler 消息到达回调函数（线程池中执行）
    void set_message_handler(std::function<void(std::string)> handler);
    
    //! @param handler 错误处理回调（在IO线程中执行）
    void set_error_handler(std::function<void(beast::error_code)> handler);

private:
    // 异步操作链：解析域名 -> 建立TCP连接 -> WebSocket握手
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_handshake(beast::error_code ec);
    
    // 持续异步读取消息
    void read_message();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    websocket::stream<tcp::socket> ws_; // WebSocket流
    tcp::resolver resolver_;            // 域名解析器
    beast::flat_buffer buffer_;         // 接收缓冲区
    ThreadPool& thread_pool_;           // 线程池引用
    std::function<void(std::string)> message_handler_;
    std::function<void(beast::error_code)> error_handler_;
};

} // namespace websocket