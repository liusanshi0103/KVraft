#include <iostream>
#include <string>

#include "clerk.h"
#include "kv_raft_cluster.h"
#include "util.h"

int main(){
KvRaftCluster cluster(3, 26000, 27000);
cluster.Start();

Clerk clerk(cluster.KvAddrs());

clerk.Put("x", "");

int follower = cluster.FindFollower();
std::cout << "stop follower=" << follower << std::endl;

cluster.StopNode(follower);

for (int i = 0; i < 20; ++i) {
  clerk.Append("x", std::to_string(i % 10));
}

cluster.RestartNode(follower);

SleepMs(5000);

std::string value = cluster.LocalGet(follower, "x");
std::cout << "follower local value=" << value << std::endl;

if (value != "01234567890123456789") {
  std::cerr << "install snapshot test failed" << std::endl;
  cluster.Stop();
  return 1;
}

std::cout << "install snapshot test passed" << std::endl;

cluster.Stop();
return 0;
}