#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

/******************************************************************************
 *
 * @file       connection_pool.h
 * @brief      连接池模板类
 *
 * @author     myself
 * @date       2025/7/12
 * @history    
 *****************************************************************************/

#include "singleton.h"
#include "logger.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <functional>

template<typename ConnectionType>
class ConnectionPool : public Singleton<ConnectionPool<ConnectionType>> {
    friend class Singleton<ConnectionPool<ConnectionType>>;
public:
    using ConnectionPtr = std::shared_ptr<ConnectionType>;
    using ConnectionFactory = std::function<ConnectionPtr()>;
    
    ~ConnectionPool();
    
    void Initialize(size_t poolSize, ConnectionFactory factory);
    ConnectionPtr GetConnection();
    void ReturnConnection(ConnectionPtr connection);
    void Close();
    
    size_t GetPoolSize() const { return _poolSize; }
    size_t GetAvailableCount() const;
    size_t GetInUseCount() const;

private:
    ConnectionPool();
    
    std::atomic<bool> _stop;
    size_t _poolSize;
    std::queue<ConnectionPtr> _connections;
    mutable std::mutex _mutex;
    std::condition_variable _condition;
    ConnectionFactory _factory;
};

template<typename ConnectionType>
ConnectionPool<ConnectionType>::ConnectionPool() 
    : _stop(false), _poolSize(0) {
}

template<typename ConnectionType>
ConnectionPool<ConnectionType>::~ConnectionPool() {
    Close();
}

template<typename ConnectionType>
void ConnectionPool<ConnectionType>::Initialize(size_t poolSize, ConnectionFactory factory) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    _poolSize = poolSize;
    _factory = factory;
    _stop = false;
    
    // 预创建连接
    for (size_t i = 0; i < _poolSize; ++i) {
        auto connection = _factory();
        if (connection) {
            _connections.push(connection);
        }
    }
    
    LOG_INFO("Connection pool initialized with {} connections", _poolSize);
}

template<typename ConnectionType>
typename ConnectionPool<ConnectionType>::ConnectionPtr 
ConnectionPool<ConnectionType>::GetConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    
    _condition.wait(lock, [this] {
        return _stop || !_connections.empty();
    });
    
    if (_stop) {
        return nullptr;
    }
    
    if (_connections.empty()) {
        // 如果池中没有连接，尝试创建新的
        if (_factory) {
            return _factory();
        }
        return nullptr;
    }
    
    auto connection = _connections.front();
    _connections.pop();
    
    LOG_DEBUG("Got connection from pool, available: {}", _connections.size());
    return connection;
}

template<typename ConnectionType>
void ConnectionPool<ConnectionType>::ReturnConnection(ConnectionPtr connection) {
    if (!connection) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (_stop) {
        return;
    }
    
    // 如果池已满，丢弃连接
    if (_connections.size() >= _poolSize) {
        LOG_DEBUG("Connection pool is full, discarding connection");
        return;
    }
    
    _connections.push(connection);
    _condition.notify_one();
    
    LOG_DEBUG("Returned connection to pool, available: {}", _connections.size());
}

template<typename ConnectionType>
void ConnectionPool<ConnectionType>::Close() {
    std::lock_guard<std::mutex> lock(_mutex);
    
    _stop = true;
    
    // 清空连接队列
    while (!_connections.empty()) {
        _connections.pop();
    }
    
    _condition.notify_all();
    
    LOG_INFO("Connection pool closed");
}

template<typename ConnectionType>
size_t ConnectionPool<ConnectionType>::GetAvailableCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _connections.size();
}

template<typename ConnectionType>
size_t ConnectionPool<ConnectionType>::GetInUseCount() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _poolSize - _connections.size();
}

#endif // CONNECTION_POOL_H 