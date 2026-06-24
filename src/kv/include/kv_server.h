#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <atomic>

#include "blocking_queue.h"
#include "raft.h"
#include "util.h"
#include "kv_service.pb.h"

class KvServer : public kvraft::KvRaftServiceRpc {
 public:
  explicit KvServer(std::shared_ptr<Raft> raft, int max_raft_state = -1);
  void Stop();
  std::string GetValueForTest(const std::string& key);
  bool PutAppendLocal(const std::string& key,
                 const std::string& value,
                 const std::string& op_type,
                 const std::string& client_id,
                 int request_id);

  std::string GetLocal(const std::string& key,
                  const std::string& client_id,
                  int request_id,
                  bool* wrong_leader);
                  void Get(google::protobuf::RpcController* controller,
         const kvraft::GetArgs* request,
         kvraft::GetReply* response,
         google::protobuf::Closure* done) override;

void PutAppend(google::protobuf::RpcController* controller,
               const kvraft::PutAppendArgs* request,
               kvraft::PutAppendReply* response,
               google::protobuf::Closure* done) override;

 private:
  void ApplyLoop();
 
  bool IsDuplicate(const std::string& client_id, int request_id);
  std::shared_ptr<BlockingQueue<Op>> GetWaitCh(int index);
  void NotifyWaitCh(int index, const Op& op);
  std::string MakeSnapshot();
  void MaybeSnapshot(int index);
  void ReadSnapshot(const std::string& snapshot);
 private:
  std::mutex mutex_;
  std::shared_ptr<Raft> raft_;

  std::unordered_map<std::string, std::string> kv_db_;
  std::unordered_map<std::string, int> last_request_id_;
  std::unordered_map<int, std::shared_ptr<BlockingQueue<Op>>> wait_apply_ch_;

  std::thread apply_thread_;
  std::atomic<bool> stopped_{false};
  int max_raft_state_=-1;
};