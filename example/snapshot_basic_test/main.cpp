#include <iostream>
#include <string>

#include "clerk.h"
#include "kv_raft_cluster.h"
#include "util.h"

int main() {
  KvRaftCluster cluster(3, 22000, 23000);
  cluster.Start();

  Clerk clerk(cluster.KvAddrs());

  for (int i = 0; i < 10; ++i) {
    clerk.Append("x", std::to_string(i));
  }

  SleepMs(2000);

  std::string value = clerk.Get("x");
  std::cout << "value=" << value << std::endl;

  int leader = cluster.FindLeader();
  int log_size = cluster.LogSize(leader);

  std::cout << "leader=" << leader
            << ", log_size=" << log_size
            << std::endl;

  if (value != "0123456789") {
    std::cerr << "snapshot basic test failed: wrong value" << std::endl;
    cluster.Stop();
    return 1;
  }

  if (log_size >= 10) {
    std::cerr << "snapshot basic test failed: log not compacted" << std::endl;
    cluster.Stop();
    return 1;
  }

  std::cout << "snapshot basic test passed" << std::endl;

  cluster.Stop();
  return 0;
}