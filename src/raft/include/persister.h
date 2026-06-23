#pragma once

#include <mutex>
#include <string>

class Persister {
 public:
  explicit Persister(const std::string& file_path);

  void SaveRaftState(const std::string& data);
  std::string ReadRaftState();
   void SaveSnapshot(const std::string& snapshot);
  std::string ReadSnapshot();

  void Save(const std::string& raft_state, const std::string& snapshot);

 private:
  std::mutex mutex_;
  std::string file_path_;
   std::string snapshot_path_;
};