#include <iostream>
#include <string>

#include "clerk.h"
#include "kv_raft_cluster.h"
#include "util.h"

int main() {
  KvRaftCluster cluster(3, 28000, 29000);
  cluster.Start();

  Clerk clerk(cluster.KvAddrs());

  clerk.Put("x", "");

  int follower = cluster.FindFollower();
  std::cout << "stop follower=" << follower << std::endl;
  cluster.StopNode(follower);

  for (int i = 0; i < 15; ++i) {
    clerk.Append("x", std::to_string(i % 10));
  }

  SleepMs(2000);
  cluster.RestartNode(follower);
SleepMs(5000);

std::string follower_value = cluster.LocalGet(follower, "x");
std::cout << "follower after restart=" << follower_value << std::endl;


  int old_leader = cluster.FindLeader();
  std::cout << "stop leader=" << old_leader << std::endl;
  cluster.StopNode(old_leader);

  int new_leader = cluster.WaitForNewLeader(old_leader, 5000);
  std::cout << "new leader=" << new_leader << std::endl;

  if (new_leader == -1) {
    std::cerr << "chaos test failed: no new leader" << std::endl;
    cluster.Stop();
    return 1;
  }

  for (int i = 15; i < 25; ++i) {
    clerk.Append("x", std::to_string(i % 10));
  }

  

  std::string value = clerk.Get("x");
  std::cout << "final value=" << value << std::endl;

  std::string expected = "0123456789012345678901234";
  if (value != expected) {
    std::cerr << "chaos test failed" << std::endl;
    cluster.Stop();
    return 1;
  }

  std::cout << "chaos basic test passed" << std::endl;

  cluster.Stop();
  return 0;
}