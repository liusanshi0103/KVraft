#include <iostream>
#include <string>

#include "clerk.h"
#include "kv_raft_cluster.h"

int main() {
  KvRaftCluster cluster(3, 21000, 21100);
  cluster.Start();

  Clerk clerk(cluster.KvAddrs());

  clerk.Put("x", "before");

  int follower = cluster.FindFollower();
  std::cout << "restart follower=" << follower << std::endl;

  if (follower == -1) {
    cluster.Stop();
    return 1;
  }

  int before_index = cluster.LastLogIndex(follower);
  std::cout << "before_index=" << before_index << std::endl;

  cluster.StopNode(follower);
  SleepMs(1000);

  cluster.RestartNode(follower);
  SleepMs(2000);

  int after_restart_index = cluster.LastLogIndex(follower);
  std::cout << "after_restart_index=" << after_restart_index << std::endl;

  clerk.Append("x", " after");

  SleepMs(1000);

  int after_append_index = cluster.LastLogIndex(follower);
  std::cout << "after_append_index=" << after_append_index << std::endl;

  std::string value = clerk.Get("x");
  std::cout << "value=" << value << std::endl;

  cluster.Stop();

  if (before_index <= 0) {
    std::cout << "full restart test failed: follower had no log before restart" << std::endl;
    return 1;
  }

  if (after_restart_index < before_index) {
    std::cout << "full restart test failed: log was not restored" << std::endl;
    return 1;
  }

  if (after_append_index <= after_restart_index) {
    std::cout << "full restart test failed: restarted node did not receive new log" << std::endl;
    return 1;
  }

  if (value != "before after") {
    std::cout << "full restart test failed: value mismatch" << std::endl;
    return 1;
  }

  std::cout << "full restart test passed" << std::endl;
  return 0;
}