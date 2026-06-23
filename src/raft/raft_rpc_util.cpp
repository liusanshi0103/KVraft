#include "raft_rpc_util.h"

RaftRpcUtil::RaftRpcUtil(const std::string& ip, uint16_t port) {
  stub_ = std::make_unique<raft::RaftRpc_Stub>(
      new MprpcChannel(ip, port));
}

bool RaftRpcUtil::RequestVote(raft::RequestVoteArgs* args,
                              raft::RequestVoteReply* reply) {
  MprpcController controller;

  stub_->RequestVote(&controller, args, reply, nullptr);

  return !controller.Failed();
}

bool RaftRpcUtil::AppendEntries(raft::AppendEntriesArgs* args,
                                raft::AppendEntriesReply* reply) {
  MprpcController controller;

  stub_->AppendEntries(&controller, args, reply, nullptr);

  return !controller.Failed();
}
bool RaftRpcUtil::InstallSnapshot(raft::InstallSnapshotArgs* args,
                                  raft::InstallSnapshotReply* reply) {
  MprpcController controller;
  stub_->InstallSnapshot(&controller, args, reply, nullptr);
  return !controller.Failed();
}