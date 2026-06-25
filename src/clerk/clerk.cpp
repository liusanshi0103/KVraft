#include "clerk.h"

#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include "mprpcconfig.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

Clerk::Clerk(const std::vector<std::pair<std::string, uint16_t>>& servers)
    : servers_(servers),
      leader_id_(0),
      request_id_(0) {
  auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  client_id_ = "client-" + std::to_string(now);
}
Clerk::Clerk(const std::string& config_file)
    : leader_id_(0),
      request_id_(0) {
  MprpcConfig config;
  if (!config.LoadConfigFile(config_file)) {
    std::cerr << "load config failed: " << config_file << std::endl;
    return;
  }

  int node_count = config.LoadInt("node_count");

  for (int i = 0; i < node_count; ++i) {
    std::string prefix = "node" + std::to_string(i);

    std::string ip = config.Load(prefix + "kvip");
    uint16_t port =
        static_cast<uint16_t>(config.LoadInt(prefix + "kvport"));

    servers_.push_back({ip, port});
  }

  auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  client_id_ = "client-" + std::to_string(now);
}
int Clerk::NextRequestId() {
  ++request_id_;
  return request_id_;
}

void Clerk::Put(const std::string& key, const std::string& value) {
  PutAppendInternal(key, value, "Put");
}

void Clerk::Append(const std::string& key, const std::string& value) {
  PutAppendInternal(key, value, "Append");
}

bool Clerk::PutAppendInternal(const std::string& key,
                              const std::string& value,
                              const std::string& op) {
  int request_id = NextRequestId();

  while (true) {
    for (int offset = 0; offset < static_cast<int>(servers_.size()); ++offset) {
      int server = (leader_id_ + offset) % servers_.size();

      kvraft::KvRaftServiceRpc_Stub stub(
          new MprpcChannel(servers_[server].first, servers_[server].second));

      kvraft::PutAppendArgs args;
      args.set_key(key);
      args.set_value(value);
      args.set_op(op);
      args.set_client_id(client_id_);
      args.set_request_id(request_id);

      kvraft::PutAppendReply reply;
      MprpcController controller;

      stub.PutAppend(&controller, &args, &reply, nullptr);

      if (controller.Failed()) {
        continue;
      }

      if (reply.err() == "OK") {
        leader_id_ = server;
        return true;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
std::string Clerk::Get(const std::string& key) {
  int request_id = NextRequestId();

  while (true) {
    for (int offset = 0; offset < static_cast<int>(servers_.size()); ++offset) {
      int server = (leader_id_ + offset) % servers_.size();

      kvraft::KvRaftServiceRpc_Stub stub(
          new MprpcChannel(servers_[server].first, servers_[server].second));

      kvraft::GetArgs args;
      args.set_key(key);
      args.set_client_id(client_id_);
      args.set_request_id(request_id);

      kvraft::GetReply reply;
      MprpcController controller;

      stub.Get(&controller, &args, &reply, nullptr);

      if (controller.Failed()) {
        continue;
      }

      if (reply.err() == "OK") {
        leader_id_ = server;
        return reply.value();
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
void Clerk::SetLeaderForTest(int leader_id) {
  if (servers_.empty()) {
    leader_id_ = 0;
    return;
  }

  leader_id_ = leader_id % static_cast<int>(servers_.size());
}