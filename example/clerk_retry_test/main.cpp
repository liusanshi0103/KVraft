#include <iostream>

#include "clerk.h"
#include "/root/code/mykvraft/example/test_util/include/kv_raft_cluster.h"

int main() {
  KvRaftCluster cluster(3, 15000, 15100);
  cluster.Start();

  Clerk clerk(cluster.KvAddrs());

  // 故意让客户端从一个不一定正确的节点开始尝试。
  clerk.SetLeaderForTest(2);

  clerk.Put("retry-key", "hello");
  clerk.Append("retry-key", " retry");

  std::string value = clerk.Get("retry-key");

  std::cout << "value=" << value << std::endl;

  cluster.Stop();

  if (value != "hello retry") {
    std::cout << "clerk retry test failed" << std::endl;
    return 1;
  }

  std::cout << "clerk retry test passed" << std::endl;
  return 0;
}