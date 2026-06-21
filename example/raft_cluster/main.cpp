#include <iostream>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "raft.h"
#include "rpcprovider.h"
#include "util.h"

int FindLeader(const std::vector<std::unique_ptr<Raft>>& rafts) {
  for (int i = 0; i < static_cast<int>(rafts.size()); ++i) {
    int term = 0;
    bool is_leader = false;
    rafts[i]->GetState(&term, &is_leader);

    if (is_leader) {
      return i;
    }
  }

  return -1;
}
int main() {
  std::vector<std::pair<std::string, uint16_t>> peer_addrs = {
      {"127.0.0.1", 9000},
      {"127.0.0.1", 9001},
      {"127.0.0.1", 9002},
  };

  std::vector<std::unique_ptr<Raft>> rafts;
  std::vector<std::unique_ptr<RpcProvider>> providers;

  for (int i = 0; i < 3; ++i) {
    rafts.push_back(std::make_unique<Raft>());
    providers.push_back(std::make_unique<RpcProvider>());
  }

  for (int i = 0; i < 3; ++i) {
    providers[i]->NotifyService(rafts[i].get());

    std::thread([&, i]() {
      providers[i]->Run(peer_addrs[i].first, peer_addrs[i].second);
    }).detach();
  }

  SleepMs(500);

  for (int i = 0; i < 3; ++i) {
    rafts[i]->Init(i, peer_addrs);
  }

  SleepMs(3000);
  int leader = FindLeader(rafts);
std::cout << "current leader: " << leader << std::endl;

if (leader != -1) {
  Op op;
  op.operation = "Put";
  op.key = "x";
  op.value = "hello";
  op.client_id = "client-1";
  op.request_id = 1;

  int log_index = -1;
  int log_term = -1;
  bool is_leader = false;

  rafts[leader]->Start(op, &log_index, &log_term, &is_leader);

SleepMs(500);

for (int i = 0; i < 3; ++i) {
  std::cout << "node " << i
            << " last_log_index="
            << rafts[i]->LastLogIndexForTest()
            << std::endl;
}
}

  for (int i = 0; i < 3; ++i) {
    ApplyMsg msg;
    int term = 0;
    bool is_leader = false;
    rafts[i]->GetState(&term, &is_leader);

    std::cout << "node " << i
              << ", term=" << term
              << ", is_leader=" << is_leader
              << std::endl;

    if(rafts[i]->PopApplyMsgForTest(&msg,100)){
      Op applied =Op::Deserialize(msg.command);
      std::cout << "node " << i
              << " apply index=" << msg.command_index
              << " op=" << applied.operation
              << " key=" << applied.key
              << " value=" << applied.value
              << std::endl;
    }
  }

  for (auto& raft : rafts) {
    raft->Stop();
  }

  return 0;
}