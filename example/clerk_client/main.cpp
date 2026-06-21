#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "clerk.h"
#include "kv_server.h"
#include "raft.h"
#include "rpcprovider.h"
#include "util.h"

int main() {
  std::vector<std::pair<std::string, uint16_t>> raft_addrs = {
      {"127.0.0.1", 13000},
      {"127.0.0.1", 13001},
      {"127.0.0.1", 13002},
  };

  std::vector<std::pair<std::string, uint16_t>> kv_addrs = {
      {"127.0.0.1", 13100},
      {"127.0.0.1", 13101},
      {"127.0.0.1", 13102},
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
      kv_providers[i]->Run(kv_addrs[i].first, kv_addrs[i].second);
    }).detach();
  }

  SleepMs(3000);

  Clerk clerk(kv_addrs);

  clerk.Put("x", "hello");
  clerk.Append("x", " world");

  std::string value = clerk.Get("x");

  std::cout << "value=" << value << std::endl;

  for (auto& raft : rafts) {
    raft->Stop();
  }

  if (value != "hello world") {
    std::cout << "clerk test failed" << std::endl;
    return 1;
  }

  std::cout << "clerk test passed" << std::endl;
  return 0;
}