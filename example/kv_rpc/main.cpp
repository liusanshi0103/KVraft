#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "kv_server.h"
#include "kv_service.pb.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"
#include "raft.h"
#include "rpcprovider.h"
#include "util.h"

int FindLeader(const std::vector<std::shared_ptr<Raft>>& rafts) {
  for (int i = 0; i < static_cast<int>(rafts.size()); ++i) {
    int term = 0;
    bool is_leader = false;

    rafts[i]->GetState(&term, &is_leader);

    std::cout << "raft node " << i
              << ", term=" << term
              << ", is_leader=" << is_leader
              << std::endl;

    if (is_leader) {
      return i;
    }
  }

  return -1;
}

bool RpcPutAppend(const std::string& ip,
                  uint16_t port,
                  const std::string& key,
                  const std::string& value,
                  const std::string& op,
                  const std::string& client_id,
                  int request_id) {
  kvraft::KvRaftServiceRpc_Stub stub(new MprpcChannel(ip, port));

  kvraft::PutAppendArgs args;
  args.set_key(key);
  args.set_value(value);
  args.set_op(op);
  args.set_client_id(client_id);
  args.set_request_id(request_id);

  kvraft::PutAppendReply reply;
  MprpcController controller;

  stub.PutAppend(&controller, &args, &reply, nullptr);

  if (controller.Failed()) {
    std::cout << "PutAppend rpc failed, port=" << port
              << ", error=" << controller.ErrorText()
              << std::endl;
    return false;
  }

  std::cout << "PutAppend port=" << port
            << ", op=" << op
            << ", err=" << reply.err()
            << std::endl;

  return reply.err() == "OK";
}

bool RpcGet(const std::string& ip,
            uint16_t port,
            const std::string& key,
            const std::string& client_id,
            int request_id,
            std::string* value) {
  kvraft::KvRaftServiceRpc_Stub stub(new MprpcChannel(ip, port));

  kvraft::GetArgs args;
  args.set_key(key);
  args.set_client_id(client_id);
  args.set_request_id(request_id);

  kvraft::GetReply reply;
  MprpcController controller;

  stub.Get(&controller, &args, &reply, nullptr);

  if (controller.Failed()) {
    std::cout << "Get rpc failed, port=" << port
              << ", error=" << controller.ErrorText()
              << std::endl;
    return false;
  }

  std::cout << "Get port=" << port
            << ", err=" << reply.err()
            << ", value=" << reply.value()
            << std::endl;

  if (reply.err() != "OK") {
    return false;
  }

  *value = reply.value();
  return true;
}

int main() {
  std::vector<std::pair<std::string, uint16_t>> raft_addrs = {
      {"127.0.0.1", 12000},
      {"127.0.0.1", 12001},
      {"127.0.0.1", 12002},
  };

  std::vector<std::pair<std::string, uint16_t>> kv_addrs = {
      {"127.0.0.1", 12100},
      {"127.0.0.1", 12101},
      {"127.0.0.1", 12102},
  };

  std::vector<std::shared_ptr<Raft>> rafts;
  std::vector<std::unique_ptr<RpcProvider>> raft_providers;

  for (int i = 0; i < 3; ++i) {
    rafts.push_back(std::make_shared<Raft>());
    raft_providers.push_back(std::make_unique<RpcProvider>());
  }

  for (int i = 0; i < 3; ++i) {
    raft_providers[i]->NotifyService(rafts[i].get());

    std::thread([&, i]() {
      std::cout << "start raft rpc provider, node=" << i
                << ", port=" << raft_addrs[i].second
                << std::endl;

      raft_providers[i]->Run(raft_addrs[i].first, raft_addrs[i].second);
    }).detach();
  }

  SleepMs(500);

  for (int i = 0; i < 3; ++i) {
    rafts[i]->Init(i, raft_addrs);
  }

  std::vector<std::unique_ptr<KvServer>> kv_servers;
  std::vector<std::unique_ptr<RpcProvider>> kv_providers;

  for (int i = 0; i < 3; ++i) {
    kv_servers.push_back(std::make_unique<KvServer>(rafts[i]));
    kv_providers.push_back(std::make_unique<RpcProvider>());
  }

  for (int i = 0; i < 3; ++i) {
    kv_providers[i]->NotifyService(kv_servers[i].get());

    std::thread([&, i]() {
      std::cout << "start kv rpc provider, node=" << i
                << ", port=" << kv_addrs[i].second
                << std::endl;

      kv_providers[i]->Run(kv_addrs[i].first, kv_addrs[i].second);
    }).detach();
  }

  SleepMs(3000);

  int leader = FindLeader(rafts);
  std::cout << "current leader=" << leader << std::endl;

  if (leader == -1) {
    std::cout << "no raft leader found" << std::endl;
    for (auto& raft : rafts) {
      raft->Stop();
    }
    return 1;
  }

  const std::string client_id = "client-1";

  bool put_ok = false;
  for (int i = 0; i < 3 && !put_ok; ++i) {
    put_ok = RpcPutAppend(kv_addrs[i].first,
                          kv_addrs[i].second,
                          "x",
                          "hello",
                          "Put",
                          client_id,
                          1);
  }

  bool append_ok = false;
  for (int i = 0; i < 3 && !append_ok; ++i) {
    append_ok = RpcPutAppend(kv_addrs[i].first,
                             kv_addrs[i].second,
                             "x",
                             " world",
                             "Append",
                             client_id,
                             2);
  }

  std::string value;
  bool get_ok = false;
  for (int i = 0; i < 3 && !get_ok; ++i) {
    get_ok = RpcGet(kv_addrs[i].first,
                    kv_addrs[i].second,
                    "x",
                    client_id,
                    3,
                    &value);
  }

  std::cout << "========== test result ==========" << std::endl;
  std::cout << "put_ok=" << put_ok << std::endl;
  std::cout << "append_ok=" << append_ok << std::endl;
  std::cout << "get_ok=" << get_ok << std::endl;
  std::cout << "value=" << value << std::endl;

  for (auto& raft : rafts) {
    raft->Stop();
  }

  if (!put_ok || !append_ok || !get_ok || value != "hello world") {
    std::cout << "kv rpc test failed" << std::endl;
    return 1;
  }

  std::cout << "kv rpc test passed" << std::endl;
  return 0;
}