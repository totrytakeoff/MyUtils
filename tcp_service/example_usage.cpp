// 示例用法：main.cpp
#include "pooled_tcp_server.hpp"
#include "tcp_session.hpp"
#include <iostream>

int main() {
    try {
        // 创建并启动TCP服务器（共享全局线程池）
        PooledTCPServer server(8080);
        
        // 设置连接处理逻辑
        server.on_connection([](tcp::socket socket) {
            std::cout << "New connection from " << socket.remote_endpoint() << std::endl;
            
            // 创建会话（需确保会话使用相同的io_context）
            auto session = std::make_shared<TCPSession>(std::move(socket));
            session->start();
        });
        
        // 启动服务器
        server.start();
        
        // 阻塞主线程等待退出（实际应用中可能需要更复杂的退出机制）
        std::cin.get();
        
        // 停止服务器
        server.stop();
        
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}