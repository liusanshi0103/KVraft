#pragma once

#include <string>
#include <unordered_map>

class MprpcConfig {
 public:
  bool LoadConfigFile(const std::string& config_file);
  std::string Load(const std::string& key) const;
  int LoadInt(const std::string& key, int default_value = 0) const;

 private:
  static void Trim(std::string* str);

 private:
  std::unordered_map<std::string, std::string> config_map_;
};