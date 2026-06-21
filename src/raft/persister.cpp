#include "persister.h"

#include <fstream>
#include <sstream>

Persister::Persister(const std::string& file_path)
    : file_path_(file_path) {}

void Persister::SaveRaftState(const std::string& data) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::ofstream ofs(file_path_, std::ios::binary | std::ios::trunc);
  ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::string Persister::ReadRaftState() {
  std::lock_guard<std::mutex> lock(mutex_);

  std::ifstream ifs(file_path_, std::ios::binary);
  if (!ifs.is_open()) {
    return "";
  }

  std::ostringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}