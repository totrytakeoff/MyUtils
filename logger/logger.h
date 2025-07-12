#ifndef LOGGER_H
#define LOGGER_H

/******************************************************************************
 *
 * @file       logger.h
 * @brief      日志管理类
 *
 * @author     myself
 * @date       2025/7/12
 * @history    
 *****************************************************************************/

#include "singleton.h"
#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger : public Singleton<Logger> {
    friend class Singleton<Logger>;
public:
    ~Logger();
    
    void Init(const std::string& logFile = "llfcchat.log", LogLevel level = LogLevel::INFO);
    void SetLogLevel(LogLevel level);
    
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);
    void Fatal(const std::string& message);
    
    template<typename... Args>
    void Debug(const std::string& format, Args... args) {
        Log(LogLevel::DEBUG, FormatMessage(format, args...));
    }
    
    template<typename... Args>
    void Info(const std::string& format, Args... args) {
        Log(LogLevel::INFO, FormatMessage(format, args...));
    }
    
    template<typename... Args>
    void Warn(const std::string& format, Args... args) {
        Log(LogLevel::WARN, FormatMessage(format, args...));
    }
    
    template<typename... Args>
    void Error(const std::string& format, Args... args) {
        Log(LogLevel::ERROR, FormatMessage(format, args...));
    }
    
    template<typename... Args>
    void Fatal(const std::string& format, Args... args) {
        Log(LogLevel::FATAL, FormatMessage(format, args...));
    }

private:
    Logger();
    void Log(LogLevel level, const std::string& message);
    std::string GetCurrentTime();
    std::string GetLevelString(LogLevel level);
    std::string FormatMessage(const std::string& format);
    
    template<typename T, typename... Args>
    std::string FormatMessage(const std::string& format, T value, Args... args) {
        size_t pos = format.find("{}");
        if (pos == std::string::npos) {
            return format;
        }
        
        std::ostringstream oss;
        oss << value;
        std::string result = format.substr(0, pos) + oss.str() + format.substr(pos + 2);
        return FormatMessage(result, args...);
    }
    
    std::ofstream _logFile;
    LogLevel _currentLevel;
    std::mutex _logMutex;
    bool _initialized;
};

// 便捷宏定义
#define LOG_DEBUG(...) Logger::GetInstance()->Debug(__VA_ARGS__)
#define LOG_INFO(...)  Logger::GetInstance()->Info(__VA_ARGS__)
#define LOG_WARN(...)  Logger::GetInstance()->Warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::GetInstance()->Error(__VA_ARGS__)
#define LOG_FATAL(...) Logger::GetInstance()->Fatal(__VA_ARGS__)

#endif // LOGGER_H 