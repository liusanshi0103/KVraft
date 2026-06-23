#include "kv_server.h"

KvServer::KvServer(std::shared_ptr<Raft> raft)
    : raft_(raft) {
  stopped_ = false;
  apply_thread_ = std::thread(&KvServer::ApplyLoop, this);
  
}
void KvServer::Stop() {
  stopped_ = true;

  if (apply_thread_.joinable()) {
    apply_thread_.join();
  }
}
void KvServer::ApplyLoop() {
  while (!stopped_) {
    ApplyMsg msg;

    if (!raft_->PopApplyMsgForTest(&msg, 100)) {
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
bool KvServer::PutAppendLocal(const std::string& key,
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
std::string KvServer::GetLocal(const std::string& key,
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
void KvServer::PutAppend(google::protobuf::RpcController* controller,
                         const kvraft::PutAppendArgs* request,
                         kvraft::PutAppendReply* response,
                         google::protobuf::Closure* done) {
  bool ok = PutAppendLocal(request->key(),
                           request->value(),
                           request->op(),
                           request->client_id(),
                           request->request_id());

  response->set_err(ok ? "OK" : "ErrWrongLeader");

  if (done) {
    done->Run();
  }
}
void KvServer::Get(google::protobuf::RpcController* controller,
                   const kvraft::GetArgs* request,
                   kvraft::GetReply* response,
                   google::protobuf::Closure* done) {
  bool wrong_leader = false;

  std::string value = GetLocal(request->key(),
                               request->client_id(),
                               request->request_id(),
                               &wrong_leader);

  if (wrong_leader) {
    response->set_err("ErrWrongLeader");
    response->set_value("");
  } else {
    response->set_err("OK");
    response->set_value(value);
  }

  if (done) {
    done->Run();
  }
}

bool KvServer::IsDuplicate(const std::string& client_id, int request_id) {
  auto it = last_request_id_.find(client_id);
  if (it == last_request_id_.end()) {
    return false;
  }
  return request_id <= it->second;
}

std::shared_ptr<BlockingQueue<Op>> KvServer::GetWaitCh(int index) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (wait_apply_ch_.find(index) == wait_apply_ch_.end()) {
    wait_apply_ch_[index] = std::make_shared<BlockingQueue<Op>>();
  }

  return wait_apply_ch_[index];
}

void KvServer::NotifyWaitCh(int index, const Op& op) {
  std::shared_ptr<BlockingQueue<Op>> ch;

  {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = wait_apply_ch_.find(index);
    if (it == wait_apply_ch_.end()) {
      return;
    }

    ch = it->second;
    wait_apply_ch_.erase(it);
  }

  ch->Push(op);
}