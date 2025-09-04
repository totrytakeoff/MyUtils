# TCP服务框架

这是一个基于Boost.Asio的TCP服务框架，提供了高性能、线程安全的TCP服务器和会话管理功能。

## 主要组件

### IOServicePool

`IOServicePool`是一个IO服务线程池，用于管理多个`boost::asio::io_context`实例，提高IO操作的并发性能。

- 单例模式设计，全局共享
- 自动负载均衡，轮询分配IO任务
- 线程安全

### TCPServer

`TCPServer`是TCP服务器类，负责接受新连接并管理所有TCP会话。

- 使用全局共享的IOServicePool
- 线程安全的会话管理
- 优雅的启动和停止机制

### TCPSession

`TCPSession`表示一个TCP连接会话，处理该连接的所有通信。

- 消息格式：4字节长度前缀 + 消息体（解决TCP粘包问题）
- 心跳机制：定期发送心跳包保持连接活跃
- 超时管理：读操作超时自动断开连接
- 线程安全的异步消息发送队列

## 使用方法

### 基本用法

```cpp
// 创建TCP服务器，自动使用全局共享的IOServicePool
TCPServer server(8080);

// 设置新连接处理回调
server.set_connection_handler([](TCPSession::Ptr session) {
    // 设置消息处理回调
    session->set_message_handler([session](const std::string& message) {
        // 处理接收到的消息
        session->send("Response");
    });
});

// 启动服务器
server.start();

// 停止服务器
server.stop();
```

### 完整示例

参见 `tcp_server_example.cpp` 文件。

## 性能优化

1. 使用IOServicePool实现IO操作的并发处理
2. 线程安全的会话管理，避免竞态条件
3. 异步操作设计，提高吞吐量
4. 心跳机制和超时管理，提高连接可靠性

## 编译要求

- C++17或更高版本
- Boost库 1.66.0或更高版本