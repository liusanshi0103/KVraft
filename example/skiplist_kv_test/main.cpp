#include <iostream>
#include <string>

#include "clerk.h"
#include "kv_raft_cluster.h"
#include "util.h"

int main() {
  KvRaftCluster cluster(3, 37000, 38000, 400);
  cluster.Start();

  Clerk clerk(cluster.KvAddrs());

  clerk.Put("name", "alice");
  clerk.Append("name", "-bob");

  std::string before = clerk.Get("name");

  if (before != "alice-bob") {
    std::cerr << "before failed, value=" << before << std::endl;
    cluster.Stop();
    return 1;
  }

  for (int i = 0; i < 30; ++i) {
    clerk.Append("name", "-x");
  }

  SleepMs(3000);

  std::string after_append = clerk.Get("name");

  cluster.RestartAll();
  SleepMs(3000);

  Clerk clerk2(cluster.KvAddrs());
  std::string after_restart = clerk2.Get("name");

  if (after_restart != after_append) {
    std::cerr << "restart failed" << std::endl;
    std::cerr << "after_append=" << after_append << std::endl;
    std::cerr << "after_restart=" << after_restart << std::endl;
    cluster.Stop();
    return 1;
  }

  cluster.Stop();
  std::cout << "skiplist kv test passed" << std::endl;
  return 0;
}