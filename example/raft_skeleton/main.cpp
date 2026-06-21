#include <iostream>

#include "raft.h"
#include "util.h"

int main() {
  Raft raft;
  raft.Init(0, 3);

  SleepMs(1200);

  int term = -1;
  bool is_leader = false;

  raft.GetState(&term, &is_leader);

  std::cout << "after wait, term: " << term << std::endl;
  std::cout << "after wait, is_leader: " << is_leader << std::endl;

  raft.Stop();

  return 0;
}