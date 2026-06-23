#include <iostream>
#include <string>

#include "clerk.h"
#include "kv_raft_cluster.h"
#include "util.h"

int main() {
  KvRaftCluster cluster(3, 24000, 25000);
  cluster.Start();

  Clerk clerk(cluster.KvAddrs());

  for (int i = 0; i < 10; ++i) {
    clerk.Append("x", std::to_string(i));
  }

  SleepMs(2000);

  std::string before = clerk.Get("x");
  std::cout << "before restart value=" << before << std::endl;

  cluster.Stop();

  cluster.RestartAll();

  Clerk clerk2(cluster.KvAddrs());

  std::string after = clerk2.Get("x");
  std::cout << "after restart value=" << after << std::endl;

  if (after != "0123456789") {
    std::cerr << "snapshot restart test failed" << std::endl;
    cluster.Stop();
    return 1;
  }

  std::cout << "snapshot restart test passed" << std::endl;

  cluster.Stop();
  return 0;
}