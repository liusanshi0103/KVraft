#include "kv_raft_cluster.h"

#include <cstdio>
#include <iostream>

#include "util.h"

KvRaftCluster::KvRaftCluster(int node_count,
                             uint16_t raft_start_port,
                             uint16_t kv_start_port)
    : node_count_(node_count) {
  for (int i = 0; i < node_count_; ++i) {
    raft_addrs_.push_back(
        {"127.0.0.1", static_cast<uint16_t>(raft_start_port + i)});

    kv_addrs_.push_back(
        {"127.0.0.1", static_cast<uint16_t>(kv_start_port + i)});
  }

  rafts_.resize(node_count_);
  kv_servers_.resize(node_count_);

  raft_providers_.resize(node_count_);
  kv_providers_.resize(node_count_);

  raft_provider_threads_.resize(node_count_);
  kv_provider_threads_.resize(node_count_);

  persisters_.resize(node_count_);
}

std::string KvRaftCluster::PersistPath(int index) const {
  return "/tmp/mykvraft-node-" + std::to_string(index) + ".bin";
}

void KvRaftCluster::Start() {
  for (int i = 0; i < node_count_; ++i) {
    std::string path = PersistPath(i);

    std::remove(path.c_str());

    persisters_[i] = std::make_shared<Persister>(path);
  }

  for (int i = 0; i < node_count_; ++i) {
    StartRaftNode(i);
  }

  SleepMs(500);

  for (int i = 0; i < node_count_; ++i) {
    rafts_[i]->Init(i, raft_addrs_, persisters_[i]);
  }

  for (int i = 0; i < node_count_; ++i) {
    StartKvNode(i);
  }

  SleepMs(3000);
}

void KvRaftCluster::StartRaftNode(int index) {
  if (raft_provider_threads_[index].joinable()) {
    raft_provider_threads_[index].join();
  }
    rafts_[index] = std::make_shared<Raft>();
  raft_providers_[index] = std::make_unique<RpcProvider>();

  raft_providers_[index]->NotifyService(rafts_[index].get());

  raft_provider_threads_[index] = std::thread([this, index]() {
    std::cout << "start raft provider node=" << index
              << ", port=" << raft_addrs_[index].second
              << std::endl;

    raft_providers_[index]->Run(raft_addrs_[index].first,
                                raft_addrs_[index].second);
  });
}

void KvRaftCluster::StartKvNode(int index) {
     if (kv_provider_threads_[index].joinable()) {
    kv_provider_threads_[index].join();
  }
  kv_servers_[index] = std::make_unique<KvServer>(rafts_[index]);
  kv_providers_[index] = std::make_unique<RpcProvider>();

  kv_providers_[index]->NotifyService(kv_servers_[index].get());

  kv_provider_threads_[index] = std::thread([this, index]() {
    std::cout << "start kv provider node=" << index
              << ", port=" << kv_addrs_[index].second
              << std::endl;

    kv_providers_[index]->Run(kv_addrs_[index].first,
                              kv_addrs_[index].second);
  });
}

void KvRaftCluster::StopNode(int index) {
  if (index < 0 || index >= node_count_) {
    return;
  }
std::cout << "[StopNode] stop raft" << std::endl;
  if (rafts_[index]) {
    rafts_[index]->Stop();
  }
std::cout << "[StopNode] stop raft provider" << std::endl;
  if (raft_providers_[index]) {
    raft_providers_[index]->Stop();
  }
  std::cout << "[StopNode] stop kv provider" << std::endl;
  if (kv_providers_[index]) {
    kv_providers_[index]->Stop();
  }
std::cout << "[StopNode] join raft provider thread" << std::endl;
  if (raft_provider_threads_[index].joinable()) {
    raft_provider_threads_[index].join();
  }
std::cout << "[StopNode] join kv provider thread" << std::endl;
  if (kv_provider_threads_[index].joinable()) {
    kv_provider_threads_[index].join();
  }
std::cout << "[StopNode] reset objects" << std::endl;
if (kv_servers_[index]) {
  kv_servers_[index]->Stop();
}
  kv_servers_[index].reset();
  rafts_[index].reset();

  kv_providers_[index].reset();
  raft_providers_[index].reset();
  std::cout << "[StopNode] done" << std::endl;
}

void KvRaftCluster::RestartNode(int index) {
  if (index < 0 || index >= node_count_) {
    return;
  }

  StartRaftNode(index);

  SleepMs(300);

  rafts_[index]->Init(index, raft_addrs_, persisters_[index]);

  StartKvNode(index);

  SleepMs(1000);
}

void KvRaftCluster::Stop() {
  for (int i = 0; i < node_count_; ++i) {
    StopNode(i);
  }
}

int KvRaftCluster::FindLeader() const {
  for (int i = 0; i < node_count_; ++i) {
    if (!rafts_[i]) {
      continue;
    }

    int term = 0;
    bool is_leader = false;

    rafts_[i]->GetState(&term, &is_leader);

    if (is_leader) {
      return i;
    }
  }

  return -1;
}

int KvRaftCluster::FindFollower() const {
  int leader = FindLeader();

  for (int i = 0; i < node_count_; ++i) {
    if (rafts_[i] && i != leader) {
      return i;
    }
  }

  return -1;
}

int KvRaftCluster::WaitForNewLeader(int old_leader, int timeout_ms) {
  int waited = 0;

  while (waited < timeout_ms) {
    int leader = FindLeader();

    if (leader != -1 && leader != old_leader) {
      return leader;
    }

    SleepMs(100);
    waited += 100;
  }

  return -1;
}

int KvRaftCluster::LastLogIndex(int index) const {
  if (index < 0 || index >= node_count_ || !rafts_[index]) {
    return -1;
  }

  return rafts_[index]->LastLogIndexForTest();
}

const std::vector<std::pair<std::string, uint16_t>>&
KvRaftCluster::KvAddrs() const {
  return kv_addrs_;
}

const std::vector<std::pair<std::string, uint16_t>>&
KvRaftCluster::RaftAddrs() const {
  return raft_addrs_;
}
int KvRaftCluster::LogSize(int index) const {
  if (index < 0 || index >= node_count_ || !rafts_[index]) {
    return -1;
  }

  return rafts_[index]->LogSizeForTest();
}
void KvRaftCluster::RestartAll() {
  for (int i = 0; i < node_count_; ++i) {
    StartRaftNode(i);
  }

  SleepMs(500);

  for (int i = 0; i < node_count_; ++i) {
    rafts_[i]->Init(i, raft_addrs_, persisters_[i]);
  }

  for (int i = 0; i < node_count_; ++i) {
    StartKvNode(i);
  }

  SleepMs(3000);
}
std::string KvRaftCluster::LocalGet(int index, const std::string& key) const {
  if (index < 0 || index >= node_count_) {
    return "";
  }

  if (!kv_servers_[index]) {
    return "";
  }

  return kv_servers_[index]->GetValueForTest(key);
}