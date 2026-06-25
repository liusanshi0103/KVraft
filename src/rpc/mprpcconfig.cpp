#include "mprpcconfig.h"

#include <fstream>

bool MprpcConfig::LoadConfigFile(const std::string& config_file) {
  std::ifstream ifs(config_file);
  if (!ifs.is_open()) {
    return false;
  }

  std::string line;
  while (std::getline(ifs, line)) {
    Trim(&line);

    if (line.empty()) {
      continue;
    }

    if (line[0] == '#') {
      continue;
    }

    size_t pos = line.find('=');
    if (pos == std::string::npos) {
      continue;
    }

    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1);

    Trim(&key);
    Trim(&value);

    if (!key.empty()) {
      config_map_[key] = value;
    }
  }

  return true;
}

std::string MprpcConfig::Load(const std::string& key) const {
  auto it = config_map_.find(key);
  if (it == config_map_.end()) {
    return "";
  }

  return it->second;
}

int MprpcConfig::LoadInt(const std::string& key, int default_value) const {
  std::string value = Load(key);
  if (value.empty()) {
    return default_value;
  }

  return std::stoi(value);
}

void MprpcConfig::Trim(std::string* str) {
  size_t start = str->find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    str->clear();
    return;
  }

  size_t end = str->find_last_not_of(" \t\r\n");
  *str = str->substr(start, end - start + 1);
}