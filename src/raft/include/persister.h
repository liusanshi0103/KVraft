#pragma once

#include <mutex>
#include <string>

class Persister {
 public:
  explicit Persister(const std::string& file_path);

  void SaveRaftState(const std::string& data);
  std::string ReadRaftState();

 private:
  std::mutex mutex_;
  std::string file_path_;
};