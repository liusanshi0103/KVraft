#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "kv_server.h"
#include "persister.h"
#include "raft.h"
#include "rpcprovider.h"

class KvRaftCluster {
 public:
  KvRaftCluster(int node_count,
                uint16_t raft_start_port,
                uint16_t kv_start_port,
                int max_raft_state=-1);
  explicit KvRaftCluster(const std::string& config_file);
  void Start();
  void Stop();

  void StopNode(int index);
  void RestartNode(int index);

  int FindLeader() const;
  int FindFollower() const;
  int WaitForNewLeader(int old_leader, int timeout_ms);
  int LastLogIndex(int index) const;
  int LogSize(int index) const;
  void RestartAll();
  std::string LocalGet(int index, const std::string& key) const;

  const std::vector<std::pair<std::string, uint16_t>>& KvAddrs() const;
  const std::vector<std::pair<std::string, uint16_t>>& RaftAddrs() const;

 private:
  std::string PersistPath(int index) const;
  void StartRaftNode(int index);
  void StartKvNode(int index);
  

 private:
  int node_count_;

  std::vector<std::pair<std::string, uint16_t>> raft_addrs_;
  std::vector<std::pair<std::string, uint16_t>> kv_addrs_;

  std::vector<std::shared_ptr<Raft>> rafts_;
  std::vector<std::unique_ptr<KvServer>> kv_servers_;

  std::vector<std::unique_ptr<RpcProvider>> raft_providers_;
  std::vector<std::unique_ptr<RpcProvider>> kv_providers_;

  std::vector<std::thread> raft_provider_threads_;
  std::vector<std::thread> kv_provider_threads_;

  std::vector<std::shared_ptr<Persister>> persisters_;
  int max_raft_state_;
};