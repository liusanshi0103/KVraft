#include <iostream>
#include <string>

#include "clerk.h"
#include "kv_raft_cluster.h"

int main() {
    const std::string config_file =
      "/root/code/mykvraft/example/config_clerk_test/test.conf";
  KvRaftCluster cluster(config_file);
  cluster.Start();

  Clerk clerk(config_file);

  clerk.Put("city", "beijing");
  clerk.Append("city", "-shanghai");

  std::string value = clerk.Get("city");

  if (value != "beijing-shanghai") {
    std::cerr << "config clerk test failed, value=" << value << std::endl;
    cluster.Stop();
    return 1;
  }

  cluster.Stop();
  std::cout << "config clerk test passed" << std::endl;
  return 0;
}