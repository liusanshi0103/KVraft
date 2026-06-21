#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "blocking_queue.h"
#include "raft.h"
#include "util.h"

class KvServer {
 public:
  explicit KvServer(std::shared_ptr<Raft> raft);

  bool PutAppend(const std::string& key,
                 const std::string& value,
                 const std::string& op_type,
                 const std::string& client_id,
                 int request_id);

  std::string Get(const std::string& key,
                  const std::string& client_id,
                  int request_id,
                  bool* wrong_leader);

 private:
  void ApplyLoop();

  bool IsDuplicate(const std::string& client_id, int request_id);
  std::shared_ptr<BlockingQueue<Op>> GetWaitCh(int index);
  void NotifyWaitCh(int index, const Op& op);

 private:
  std::mutex mutex_;
  std::shared_ptr<Raft> raft_;

  std::unordered_map<std::string, std::string> kv_db_;
  std::unordered_map<std::string, int> last_request_id_;
  std::unordered_map<int, std::shared_ptr<BlockingQueue<Op>>> wait_apply_ch_;

  std::thread apply_thread_;
};