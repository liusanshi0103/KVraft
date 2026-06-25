#include "kv_server.h"

KvServer::KvServer(std::shared_ptr<Raft> raft, int max_raft_state)
    : raft_(raft),
    kv_db_(std::make_unique<SkipList<std::string,std::string>>(6)),
     max_raft_state_(max_raft_state) {
  ReadSnapshot(raft_->ReadSnapshot());
  stopped_ = false;
  apply_thread_ = std::thread(&KvServer::ApplyLoop, this);
  
}
std::string KvServer::GetValueForTest(const std::string& key) {
  std::lock_guard<std::mutex> lock(mutex_);

std::string value;
if (!kv_db_->search_element(key, value)) {
  return "";
}

return value;


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
    if (msg.snapshot_valid) {
    ReadSnapshot(msg.snapshot);
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
    std::string key = op.key;
   std::string value = op.value;

   kv_db_->insert_set_element(key, value);
    last_request_id_[op.client_id] = op.request_id;
  } else if (op.operation == "Append") {
    std::string old_value;
    kv_db_->search_element(op.key, old_value);

    std::string key = op.key;
    std::string new_value = old_value + op.value;

    kv_db_->insert_set_element(key, new_value);
    last_request_id_[op.client_id] = op.request_id;
}
      }
    }

    NotifyWaitCh(msg.command_index, op);
    MaybeSnapshot(msg.command_index);
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

  std::string value;
  if (!kv_db_->search_element(key, value)) {
  return "";
 }

    return value;
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
namespace {

void AppendString(std::string* out, const std::string& value) {
  *out += std::to_string(value.size());
  *out += "\n";
  *out += value;
  *out += "\n";
}

}  // namespace
std::string KvServer::MakeSnapshot() {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string data;

  AppendString(&data, kv_db_->dump_file());

  data += std::to_string(last_request_id_.size());
  data += "\n";

  for (const auto& [client_id, request_id] : last_request_id_) {
    AppendString(&data, client_id);
    data += std::to_string(request_id);
    data += "\n";
  }

  return data;
}
void KvServer::MaybeSnapshot(int index) {
  if (index <= 0) {
    return;
  }

  if (max_raft_state_== -1) {
    return;
  }
  if (raft_->GetRaftStateSize() < max_raft_state_) {
    return;
  }

  std::string snapshot = MakeSnapshot();
  raft_->Snapshot(index, snapshot);
}
namespace {

bool ReadLine(const std::string& data, size_t* pos, std::string* line) {
  size_t end = data.find('\n', *pos);
  if (end == std::string::npos) {
    return false;
  }

  *line = data.substr(*pos, end - *pos);
  *pos = end + 1;
  return true;
}

bool ReadString(const std::string& data, size_t* pos, std::string* value) {
  std::string size_line;
  if (!ReadLine(data, pos, &size_line)) {
    return false;
  }

  int size = std::stoi(size_line);
  if (*pos + size > data.size()) {
    return false;
  }

  *value = data.substr(*pos, size);
  *pos += size;

  if (*pos < data.size() && data[*pos] == '\n') {
    ++(*pos);
  }

  return true;
}

}  // namespace
void KvServer::ReadSnapshot(const std::string& snapshot) {
  if (snapshot.empty()) {
    return;
  }

  size_t pos = 0;
  std::string kv_dump;
    if (!ReadString(snapshot, &pos, &kv_dump)) {
    return;
  }
   auto new_kv_db =
    std::make_unique<SkipList<std::string, std::string>>(6);
    new_kv_db->load_file(kv_dump);
  std::unordered_map<std::string, int> new_last_request_id;
  std::string line;
  if (!ReadLine(snapshot, &pos, &line)) {
    return;
  }

  int request_size = std::stoi(line);


  for (int i = 0; i < request_size; ++i) {
    std::string client_id;
    std::string request_id_line;

    if (!ReadString(snapshot, &pos, &client_id)) {
      return;
    }

    if (!ReadLine(snapshot, &pos, &request_id_line)) {
      return;
    }

    new_last_request_id[client_id] = std::stoi(request_id_line);
  }

  std::lock_guard<std::mutex> lock(mutex_);
  kv_db_ = std::move(new_kv_db);
  last_request_id_ = std::move(new_last_request_id);
}