#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger::Logger() : _currentLevel(LogLevel::INFO), _initialized(false) {
}

Logger::~Logger() {
    if (_logFile.is_open()) {
        _logFile.close();
    }
}

void Logger::Init(const std::string& logFile, LogLevel level) {
    std::lock_guard<std::mutex> lock(_logMutex);
    
    if (_initialized) {
        return;
    }
    
    _logFile.open(logFile, std::ios::app);
    _currentLevel = level;
    _initialized = true;
    
    Info("Logger initialized with level: {}", static_cast<int>(level));
}

void Logger::SetLogLevel(LogLevel level) {
    _currentLevel = level;
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::DEBUG, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::INFO, message);
}

void Logger::Warn(const std::string& message) {
    Log(LogLevel::WARN, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::ERROR, message);
}

void Logger::Fatal(const std::string& message) {
    Log(LogLevel::FATAL, message);
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < _currentLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(_logMutex);
    
    std::string logMessage = GetCurrentTime() + " [" + GetLevelString(level) + "] " + message + "\n";
    
    // 输出到控制台
    std::cout << logMessage;
    
    // 输出到文件
    if (_logFile.is_open()) {
        _logFile << logMessage;
        _logFile.flush();
    }
}

std::string Logger::GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::GetLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::FormatMessage(const std::string& format) {
    return format;
} 