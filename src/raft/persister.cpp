#include "persister.h"

#include <fstream>
#include <sstream>

namespace {

void WriteFile(const std::string& path, const std::string& data) {
  std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
  ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::string ReadFile(const std::string& path) {
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs.is_open()) {
    return "";
  }

  std::ostringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

}  // namespace

Persister::Persister(const std::string& file_path)
    : file_path_(file_path),
      snapshot_path_(file_path + ".snapshot") {}

void Persister::SaveRaftState(const std::string& data) {
  std::lock_guard<std::mutex> lock(mutex_);
  WriteFile(file_path_, data);
}

std::string Persister::ReadRaftState() {
  std::lock_guard<std::mutex> lock(mutex_);
  return ReadFile(file_path_);
}

void Persister::SaveSnapshot(const std::string& snapshot) {
  std::lock_guard<std::mutex> lock(mutex_);
  WriteFile(snapshot_path_, snapshot);
}

std::string Persister::ReadSnapshot() {
  std::lock_guard<std::mutex> lock(mutex_);
  return ReadFile(snapshot_path_);
}

void Persister::Save(const std::string& raft_state,
                     const std::string& snapshot) {
  std::lock_guard<std::mutex> lock(mutex_);
  WriteFile(file_path_, raft_state);
  WriteFile(snapshot_path_, snapshot);
}
long long Persister::RaftStateSize() {
  std::lock_guard<std::mutex> lock(mutex_);

  std::ifstream ifs(file_path_, std::ios::binary | std::ios::ate);
  if (!ifs.is_open()) {
    return 0;
  }

  return static_cast<long long>(ifs.tellg());
}