#include <iostream>
#include <string>

#include "clerk.h"
#include "kv_raft_cluster.h"
#include "util.h"

int main() {
  KvRaftCluster cluster(3, 27000, 28000, 400);
  cluster.Start();

  Clerk clerk(cluster.KvAddrs());

  for (int i = 0; i < 30; ++i) {
    clerk.Append("k", "0123456789");
  }

  SleepMs(3000);

  std::string value = clerk.Get("k");
  int leader = cluster.FindLeader();

  std::cout << "leader=" << leader << std::endl;
  std::cout << "value_size=" << value.size() << std::endl;

  for (int i = 0; i < 3; ++i) {
    std::cout << "node " << i
              << " log_size=" << cluster.LogSize(i)
              << " last_log_index=" << cluster.LastLogIndex(i)
              << std::endl;
  }

  if (value.size() != 300) {
    std::cerr << "snapshot size test failed: wrong value size" << std::endl;
    cluster.Stop();
    return 1;
  }

  cluster.Stop();
  std::cout << "snapshot size test passed" << std::endl;
  return 0;
}