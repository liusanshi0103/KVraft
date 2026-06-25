#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <unistd.h>

#include "kv_server.h"
#include "mprpcconfig.h"
#include "persister.h"
#include "raft.h"
#include "rpcprovider.h"
#include "util.h"

std::atomic<bool> g_stopped{false};

void OnSignal(int) {
  g_stopped = true;
}

void ShowUsage() {
  std::cout << "usage: raft_kv_node -i <node_id> -f <config_file>\n";
}

int main(int argc, char** argv) {
  int me = -1;
  std::string config_file;

  int c = 0;
  while ((c = getopt(argc, argv, "i:f:")) != -1) {
    switch (c) {
      case 'i':
        me = std::atoi(optarg);
        break;
      case 'f':
        config_file = optarg;
        break;
      default:
        ShowUsage();
        return 1;
    }
  }

  if (me < 0 || config_file.empty()) {
    ShowUsage();
    return 1;
  }

  std::signal(SIGINT, OnSignal);
  std::signal(SIGTERM, OnSignal);

  MprpcConfig config;
  if (!config.LoadConfigFile(config_file)) {
    std::cerr << "load config failed: " << config_file << std::endl;
    return 1;
  }

  int node_count = config.LoadInt("node_count");
  int max_raft_state = config.LoadInt("max_raft_state", -1);

  if (me >= node_count) {
    std::cerr << "bad node id: " << me << std::endl;
    return 1;
  }

  std::vector<std::pair<std::string, uint16_t>> raft_addrs;

  for (int i = 0; i < node_count; ++i) {
    std::string prefix = "node" + std::to_string(i);

    std::string ip = config.Load(prefix + "raftip");
    uint16_t port =
        static_cast<uint16_t>(config.LoadInt(prefix + "raftport"));

    raft_addrs.push_back({ip, port});
  }

  std::string prefix = "node" + std::to_string(me);
  std::string kv_ip = config.Load(prefix + "kvip");
  uint16_t kv_port =
      static_cast<uint16_t>(config.LoadInt(prefix + "kvport"));

  std::string persist_path =
      "/tmp/mykvraft-real-node-" + std::to_string(me) + ".bin";

  auto persister = std::make_shared<Persister>(persist_path);

  auto raft = std::make_shared<Raft>();
  raft->Init(me, raft_addrs, persister);

  KvServer kv_server(raft, max_raft_state);

  RpcProvider raft_provider;
  RpcProvider kv_provider;

  raft_provider.NotifyService(raft.get());
  kv_provider.NotifyService(&kv_server);

  std::thread raft_thread([&]() {
    std::cout << "raft node " << me
              << " listen on " << raft_addrs[me].first
              << ":" << raft_addrs[me].second << std::endl;

    raft_provider.Run(raft_addrs[me].first, raft_addrs[me].second);
  });

  std::thread kv_thread([&]() {
    std::cout << "kv node " << me
              << " listen on " << kv_ip
              << ":" << kv_port << std::endl;

    kv_provider.Run(kv_ip, kv_port);
  });

  while (!g_stopped) {
    SleepMs(1000);
  }

  raft->Stop();
  kv_server.Stop();
  raft_provider.Stop();
  kv_provider.Stop();

  if (raft_thread.joinable()) {
    raft_thread.join();
  }

  if (kv_thread.joinable()) {
    kv_thread.join();
  }

  return 0;
}