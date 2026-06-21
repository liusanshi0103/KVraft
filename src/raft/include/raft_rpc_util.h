#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "mprpcchannel.h"
#include "mprpccontroller.h"
#include "raft.pb.h"

class RaftRpcUtil {
 public:
  RaftRpcUtil(const std::string& ip, uint16_t port);

  bool RequestVote(raft::RequestVoteArgs* args,
                   raft::RequestVoteReply* reply);

  bool AppendEntries(raft::AppendEntriesArgs* args,
                     raft::AppendEntriesReply* reply);

 private:
  std::unique_ptr<raft::RaftRpc_Stub> stub_;
};