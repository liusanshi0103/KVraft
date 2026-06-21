#include "kv_server.h"

KvServer::KvServer(std::shared_ptr<Raft> raft)
    : raft_(raft) {
  apply_thread_ = std::thread(&KvServer::ApplyLoop, this);
  apply_thread_.detach();
}
void KvServer::ApplyLoop() {
  while (true) {
    ApplyMsg msg;

    if (!raft_->PopApplyMsg(&msg, 100)) {
      continue;
    }

    if (!msg.command_valid) {
      continue;
    }

    Op op = Op::Deserialize(msg.command);

    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (!IsDuplicate(op.client_id, op.request_id)) {
        if (op.operation == "Put") {
          kv_db_[op.key] = op.value;
          last_request_id_[op.client_id] = op.request_id;
        } else if (op.operation == "Append") {
          kv_db_[op.key] += op.value;
          last_request_id_[op.client_id] = op.request_id;
        }
      }
    }

    NotifyWaitCh(msg.command_index, op);
  }
}
bool KvServer::PutAppend(const std::string& key,
                         const std::string& value,
                         const std::string& op_type,
                         const std::string& client_id,
                         int request_id) {
  Op op;
  op.operation = op_type;
  op.key = key;
  op.value = value;
  op.client_id = client_id;
  op.request_id = request_id;

  int index = -1;
  int term = -1;
  bool is_leader = false;

  raft_->Start(op, &index, &term, &is_leader);

  if (!is_leader) {
    return false;
  }

  auto ch = GetWaitCh(index);

  Op applied_op;
  if (!ch->Pop(&applied_op, 1000)) {
    return IsDuplicate(client_id, request_id);
  }

  return applied_op.client_id == client_id &&
         applied_op.request_id == request_id;
}
std::string KvServer::Get(const std::string& key,
                          const std::string& client_id,
                          int request_id,
                          bool* wrong_leader) {
  Op op;
  op.operation = "Get";
  op.key = key;
  op.client_id = client_id;
  op.request_id = request_id;

  int index = -1;
  int term = -1;
  bool is_leader = false;

  raft_->Start(op, &index, &term, &is_leader);

  if (!is_leader) {
    *wrong_leader = true;
    return "";
  }

  auto ch = GetWaitCh(index);

  Op applied_op;
  if (!ch->Pop(&applied_op, 1000)) {
    *wrong_leader = true;
    return "";
  }

  std::lock_guard<std::mutex> lock(mutex_);
  *wrong_leader = false;

  auto it = kv_db_.find(key);
  if (it == kv_db_.end()) {
    return "";
  }

  return it->second;
}