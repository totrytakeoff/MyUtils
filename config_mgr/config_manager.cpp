#include "config_manager.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

ConfigManager::ConfigManager() : _loaded(false) {
}

ConfigManager::~ConfigManager() {
}

bool ConfigManager::LoadConfig(const std::string& configFile) {
    _config_file = configFile;
    return ReloadConfig();
}

void ConfigManager::ReloadConfig() {
    if (_config_file.empty()) {
        LOG_ERROR("Config file path is empty");
        return;
    }
    
    std::ifstream file(_config_file);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open config file: {}", _config_file);
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    if (ParseIniFile(buffer.str())) {
        _loaded = true;
        LOG_INFO("Config loaded successfully from: {}", _config_file);
        PrintConfig();
    } else {
        LOG_ERROR("Failed to parse config file: {}", _config_file);
    }
}

bool ConfigManager::ParseIniFile(const std::string& content) {
    _config_map.clear();
    
    std::istringstream stream(content);
    std::string line;
    std::string currentSection;
    
    while (std::getline(stream, line)) {
        // 去除行首行尾空白字符
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // 跳过空行和注释行
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        // 处理节名 [section]
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        // 处理键值对
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除键值对的空白字符
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (!key.empty()) {
                _config_map[currentSection]._section_datas[key] = value;
            }
        }
    }
    
    return true;
}

SectionInfo ConfigManager::operator[](const std::string& section) const {
    auto it = _config_map.find(section);
    if (it == _config_map.end()) {
        return SectionInfo();
    }
    return it->second;
}

std::string ConfigManager::GetValue(const std::string& section, const std::string& key) const {
    return (*this)[section].GetValue(key);
}

bool ConfigManager::HasSection(const std::string& section) const {
    return _config_map.find(section) != _config_map.end();
}

bool ConfigManager::HasKey(const std::string& section, const std::string& key) const {
    return (*this)[section].HasKey(key);
}

int ConfigManager::GetInt(const std::string& section, const std::string& key, int defaultValue) const {
    std::string value = GetValue(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    
    try {
        return std::stoi(value);
    } catch (const std::exception& e) {
        LOG_WARN("Failed to convert '{}' to int in section [{}], key '{}', using default: {}", 
                 value, section, key, defaultValue);
        return defaultValue;
    }
}

double ConfigManager::GetDouble(const std::string& section, const std::string& key, double defaultValue) const {
    std::string value = GetValue(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    
    try {
        return std::stod(value);
    } catch (const std::exception& e) {
        LOG_WARN("Failed to convert '{}' to double in section [{}], key '{}', using default: {}", 
                 value, section, key, defaultValue);
        return defaultValue;
    }
}

bool ConfigManager::GetBool(const std::string& section, const std::string& key, bool defaultValue) const {
    std::string value = GetValue(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    
    // 转换为小写进行比较
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    } else if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    } else {
        LOG_WARN("Failed to convert '{}' to bool in section [{}], key '{}', using default: {}", 
                 value, section, key, defaultValue);
        return defaultValue;
    }
}

void ConfigManager::PrintConfig() const {
    LOG_INFO("=== Configuration ===");
    for (const auto& section : _config_map) {
        LOG_INFO("[{}]", section.first);
        for (const auto& kv : section.second._section_datas) {
            LOG_INFO("  {} = {}", kv.first, kv.second);
        }
    }
    LOG_INFO("====================");
} 