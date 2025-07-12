#include "connection_pool.h"
#include "logger.h"
#include <iostream>
#include <string>
#include <memory>

// ==================== MySQL 连接示例 ====================

class MySQLConnection {
public:
    MySQLConnection(const std::string& url, const std::string& user, 
                   const std::string& pass, const std::string& schema)
        : _url(url), _user(user), _pass(pass), _schema(schema), _isValid(false) {
        try {
            // 这里应该使用实际的 MySQL 驱动
            // sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            // _connection = std::unique_ptr<sql::Connection>(driver->connect(url, user, pass));
            // _connection->setSchema(schema);
            _isValid = true;
            LOG_INFO("MySQL connection created successfully");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create MySQL connection: {}", e.what());
            _isValid = false;
        }
    }
    
    bool Execute(const std::string& sql) {
        if (!_isValid) return false;
        
        try {
            // 实际执行 SQL
            // std::unique_ptr<sql::Statement> stmt(_connection->createStatement());
            // stmt->execute(sql);
            LOG_DEBUG("Executed SQL: {}", sql);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("SQL execution failed: {}", e.what());
            _isValid = false;
            return false;
        }
    }
    
    bool IsValid() const { return _isValid; }
    
    void Close() {
        _isValid = false;
        // _connection.reset();
        LOG_DEBUG("MySQL connection closed");
    }
    
    // 健康检查
    bool Ping() {
        return Execute("SELECT 1");
    }
    
private:
    std::string _url, _user, _pass, _schema;
    // std::unique_ptr<sql::Connection> _connection;
    bool _isValid;
};

// ==================== Redis 连接示例 ====================

class RedisConnection {
public:
    RedisConnection(const std::string& host, int port, const std::string& password)
        : _host(host), _port(port), _password(password), _context(nullptr), _isValid(false) {
        try {
            // 这里应该使用实际的 Redis 客户端
            // _context = redisConnect(host.c_str(), port);
            // if (_context && _context->err == 0) {
            //     if (!password.empty()) {
            //         auto reply = (redisReply*)redisCommand(_context, "AUTH %s", password.c_str());
            //         if (reply->type == REDIS_REPLY_ERROR) {
            //             freeReplyObject(reply);
            //             throw std::runtime_error("Redis authentication failed");
            //         }
            //         freeReplyObject(reply);
            //     }
            //     _isValid = true;
            // }
            _isValid = true;
            LOG_INFO("Redis connection created successfully");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create Redis connection: {}", e.what());
            _isValid = false;
        }
    }
    
    bool Execute(const std::string& command) {
        if (!_isValid) return false;
        
        try {
            // 实际执行 Redis 命令
            // auto reply = (redisReply*)redisCommand(_context, command.c_str());
            // if (reply) {
            //     freeReplyObject(reply);
            //     return true;
            // }
            LOG_DEBUG("Executed Redis command: {}", command);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Redis command failed: {}", e.what());
            _isValid = false;
            return false;
        }
    }
    
    bool IsValid() const { return _isValid; }
    
    void Close() {
        _isValid = false;
        // if (_context) {
        //     redisFree(_context);
        //     _context = nullptr;
        // }
        LOG_DEBUG("Redis connection closed");
    }
    
    // 健康检查
    bool Ping() {
        return Execute("PING");
    }
    
private:
    std::string _host, _password;
    int _port;
    // redisContext* _context;
    bool _isValid;
};

// ==================== HTTP 连接示例 ====================

class HttpConnection {
public:
    HttpConnection(const std::string& host, int port)
        : _host(host), _port(port), _isValid(false) {
        try {
            // 这里应该使用实际的 HTTP 客户端库
            // 例如 Boost.Beast 或 libcurl
            _isValid = true;
            LOG_INFO("HTTP connection created successfully");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create HTTP connection: {}", e.what());
            _isValid = false;
        }
    }
    
    bool Execute(const std::string& request) {
        if (!_isValid) return false;
        
        try {
            // 实际发送 HTTP 请求
            LOG_DEBUG("Sent HTTP request: {}", request);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("HTTP request failed: {}", e.what());
            _isValid = false;
            return false;
        }
    }
    
    bool IsValid() const { return _isValid; }
    
    void Close() {
        _isValid = false;
        LOG_DEBUG("HTTP connection closed");
    }
    
private:
    std::string _host;
    int _port;
    bool _isValid;
};

// ==================== 使用示例 ====================

void MySQLPoolExample() {
    std::cout << "\n=== MySQL 连接池示例 ===" << std::endl;
    
    // 获取 MySQL 连接池实例
    auto& mysqlPool = ConnectionPool<MySQLConnection>::GetInstance();
    
    // 初始化连接池
    mysqlPool.Initialize(5, []() {
        return std::make_shared<MySQLConnection>("localhost:3306", "user", "password", "mydb");
    });
    
    // 使用连接
    for (int i = 0; i < 10; ++i) {
        auto conn = mysqlPool.GetConnection();
        if (conn) {
            std::cout << "Got MySQL connection, executing query..." << std::endl;
            conn->Execute("SELECT * FROM users LIMIT 10");
            mysqlPool.ReturnConnection(conn);
        }
    }
    
    std::cout << "Available connections: " << mysqlPool.GetAvailableCount() << std::endl;
    std::cout << "In-use connections: " << mysqlPool.GetInUseCount() << std::endl;
}

void RedisPoolExample() {
    std::cout << "\n=== Redis 连接池示例 ===" << std::endl;
    
    // 获取 Redis 连接池实例
    auto& redisPool = ConnectionPool<RedisConnection>::GetInstance();
    
    // 初始化连接池
    redisPool.Initialize(3, []() {
        return std::make_shared<RedisConnection>("localhost", 6379, "password");
    });
    
    // 使用连接
    for (int i = 0; i < 8; ++i) {
        auto conn = redisPool.GetConnection();
        if (conn) {
            std::cout << "Got Redis connection, executing command..." << std::endl;
            conn->Execute("SET key" + std::to_string(i) + " value" + std::to_string(i));
            redisPool.ReturnConnection(conn);
        }
    }
    
    std::cout << "Available connections: " << redisPool.GetAvailableCount() << std::endl;
    std::cout << "In-use connections: " << redisPool.GetInUseCount() << std::endl;
}

void HttpPoolExample() {
    std::cout << "\n=== HTTP 连接池示例 ===" << std::endl;
    
    // 获取 HTTP 连接池实例
    auto& httpPool = ConnectionPool<HttpConnection>::GetInstance();
    
    // 初始化连接池
    httpPool.Initialize(4, []() {
        return std::make_shared<HttpConnection>("api.example.com", 80);
    });
    
    // 使用连接
    for (int i = 0; i < 6; ++i) {
        auto conn = httpPool.GetConnection();
        if (conn) {
            std::cout << "Got HTTP connection, sending request..." << std::endl;
            conn->Execute("GET /api/data HTTP/1.1");
            httpPool.ReturnConnection(conn);
        }
    }
    
    std::cout << "Available connections: " << httpPool.GetAvailableCount() << std::endl;
    std::cout << "In-use connections: " << httpPool.GetInUseCount() << std::endl;
}

// ==================== 高级用法示例 ====================

class DatabaseManager {
private:
    ConnectionPool<MySQLConnection> _mysqlPool;
    ConnectionPool<RedisConnection> _redisPool;
    
public:
    DatabaseManager() {
        // 初始化 MySQL 连接池
        _mysqlPool.Initialize(5, []() {
            return std::make_shared<MySQLConnection>("localhost:3306", "user", "pass", "db");
        });
        
        // 初始化 Redis 连接池
        _redisPool.Initialize(3, []() {
            return std::make_shared<RedisConnection>("localhost", 6379, "pass");
        });
    }
    
    // 事务操作示例
    bool ProcessUserRegistration(const std::string& username, const std::string& email) {
        // 获取 MySQL 连接
        auto mysqlConn = _mysqlPool.GetConnection();
        if (!mysqlConn) {
            LOG_ERROR("Failed to get MySQL connection");
            return false;
        }
        
        // 获取 Redis 连接
        auto redisConn = _redisPool.GetConnection();
        if (!redisConn) {
            _mysqlPool.ReturnConnection(mysqlConn);
            LOG_ERROR("Failed to get Redis connection");
            return false;
        }
        
        try {
            // 在 MySQL 中插入用户
            mysqlConn->Execute("INSERT INTO users (username, email) VALUES ('" + username + "', '" + email + "')");
            
            // 在 Redis 中缓存用户信息
            redisConn->Execute("SET user:" + username + " " + email);
            
            // 归还连接
            _mysqlPool.ReturnConnection(mysqlConn);
            _redisPool.ReturnConnection(redisConn);
            
            LOG_INFO("User registration successful: {}", username);
            return true;
            
        } catch (const std::exception& e) {
            // 归还连接
            _mysqlPool.ReturnConnection(mysqlConn);
            _redisPool.ReturnConnection(redisConn);
            
            LOG_ERROR("User registration failed: {}", e.what());
            return false;
        }
    }
    
    void Close() {
        _mysqlPool.Close();
        _redisPool.Close();
    }
};

int main() {
    std::cout << "=== 连接池使用示例 ===" << std::endl;
    
    // 基本使用示例
    MySQLPoolExample();
    RedisPoolExample();
    HttpPoolExample();
    
    // 高级用法示例
    std::cout << "\n=== 高级用法示例 ===" << std::endl;
    DatabaseManager dbManager;
    dbManager.ProcessUserRegistration("testuser", "test@example.com");
    dbManager.Close();
    
    std::cout << "\n=== 示例完成 ===" << std::endl;
    return 0;
} 