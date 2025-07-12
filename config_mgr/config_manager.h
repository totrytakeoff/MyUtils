#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

/******************************************************************************
 *
 * @file       config_manager.h
 * @brief      配置管理类
 *
 * @author     myself
 * @date       2025/7/12
 * @history    
 *****************************************************************************/

#include "singleton.h"
#include <string>
#include <map>
#include <memory>

struct SectionInfo {
    SectionInfo() = default;
    ~SectionInfo() = default;
    
    SectionInfo(const SectionInfo& src) {
        _section_datas = src._section_datas;
    }
    
    SectionInfo& operator = (const SectionInfo& src) {
        if (&src == this) {
            return *this;
        }
        this->_section_datas = src._section_datas;
        return *this;
    }

    std::map<std::string, std::string> _section_datas;
    
    std::string operator[](const std::string& key) const {
        auto it = _section_datas.find(key);
        if (it == _section_datas.end()) {
            return "";
        }
        return it->second;
    }

    std::string GetValue(const std::string& key) const {
        return (*this)[key];
    }
    
    bool HasKey(const std::string& key) const {
        return _section_datas.find(key) != _section_datas.end();
    }
};

class ConfigManager : public Singleton<ConfigManager> {
    friend class Singleton<ConfigManager>;
public:
    ~ConfigManager();
    
    bool LoadConfig(const std::string& configFile);
    void ReloadConfig();
    
    SectionInfo operator[](const std::string& section) const;
    std::string GetValue(const std::string& section, const std::string& key) const;
    bool HasSection(const std::string& section) const;
    bool HasKey(const std::string& section, const std::string& key) const;
    
    // 便捷方法
    int GetInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    double GetDouble(const std::string& section, const std::string& key, double defaultValue = 0.0) const;
    bool GetBool(const std::string& section, const std::string& key, bool defaultValue = false) const;
    
    void PrintConfig() const;

private:
    ConfigManager();
    bool ParseIniFile(const std::string& content);
    
    std::map<std::string, SectionInfo> _config_map;
    std::string _config_file;
    bool _loaded;
};

#endif // CONFIG_MANAGER_H 