#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "persister.h"
#include "raft.h"
#include "util.h"

int main() {
  const std::string path = "/tmp/mykvraft-persister-test.bin";

  std::vector<std::pair<std::string, uint16_t>> addrs = {
      {"127.0.0.1", 18000},
  };

  {
    auto persister = std::make_shared<Persister>(path);
    Raft raft;
    raft.Init(0, addrs, persister);

    Op op;
    op.operation = "Put";
    op.key = "x";
    op.value = "persist";
    op.client_id = "client-1";
    op.request_id = 1;

    int index = -1;
    int term = -1;
    bool is_leader = false;

    // 单节点测试里，如果你当前 Raft 不会自动成为 leader，
    // 可以临时加一个 BecomeLeaderForTest()。
    raft.BecomeLeaderForTest();

    raft.Start(op, &index, &term, &is_leader);

    std::cout << "first raft start index=" << index
              << ", term=" << term
              << ", is_leader=" << is_leader
              << std::endl;

    raft.Stop();
  }

  {
    auto persister = std::make_shared<Persister>(path);
    Raft raft;
    raft.Init(0, addrs, persister);

    int last_log_index = raft.LastLogIndexForTest();

    std::cout << "restored last_log_index=" << last_log_index << std::endl;

    raft.Stop();

    if (last_log_index != 1) {
      std::cout << "persister test failed" << std::endl;
      return 1;
    }
  }

  std::cout << "persister test passed" << std::endl;
  return 0;
}