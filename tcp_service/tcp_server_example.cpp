#include "tcp_server.hpp"
#include "IOService_pool.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    try {
        // 初始化全局IOServicePool（可选，如果不初始化，将使用默认线程数）
        // 这一步不是必须的，因为TCPServer会在需要时自动初始化
        IOServicePool::GetInstance();
        
        // 创建TCP服务器，自动使用全局IOServicePool
        TCPServer server(8080);
        
        // 设置新连接处理回调
        server.set_connection_handler([](TCPSession::Ptr session) {
            std::cout << "New connection established: " << session->remote_endpoint() << std::endl;
            
            // 设置消息处理回调
            session->set_message_handler([session](const std::string& message) {
                std::cout << "Received message from " << session->remote_endpoint() 
                          << ": " << message << std::endl;
                
                // 回显消息
                session->send("Echo: " + message);
            });
        });
        
        // 启动服务器
        server.start();
        
        std::cout << "Server started on port 8080. Press Enter to exit." << std::endl;
        std::cin.get();
        
        // 停止服务器
        server.stop();
        
        // 停止全局IOServicePool
        IOServicePool::GetInstance().Stop();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}