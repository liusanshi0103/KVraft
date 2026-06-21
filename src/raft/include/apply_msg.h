#pragma once

#include <string>

struct ApplyMsg {
  bool command_valid = false;
  std::string command;
  int command_index = -1;

  bool snapshot_valid = false;
  std::string snapshot;
  int snapshot_term = -1;
  int snapshot_index = -1;
};