#include <iostream>
#include <string>

#include "clerk.h"

void ShowUsage() {
  std::cout << "usage:\n";
  std::cout << "  raft_kv_client <config_file> put <key> <value>\n";
  std::cout << "  raft_kv_client <config_file> append <key> <value>\n";
  std::cout << "  raft_kv_client <config_file> get <key>\n";
}

int main(int argc, char** argv) {
  if (argc < 4) {
    ShowUsage();
    return 1;
  }

  std::string config_file = argv[1];
  std::string op = argv[2];
  std::string key = argv[3];

  Clerk clerk(config_file);

  if (op == "put") {
    if (argc != 5) {
      ShowUsage();
      return 1;
    }

    clerk.Put(key, argv[4]);
    std::cout << "OK" << std::endl;
    return 0;
  }

  if (op == "append") {
    if (argc != 5) {
      ShowUsage();
      return 1;
    }

    clerk.Append(key, argv[4]);
    std::cout << "OK" << std::endl;
    return 0;
  }

  if (op == "get") {
    std::cout << clerk.Get(key) << std::endl;
    return 0;
  }

  ShowUsage();
  return 1;
}