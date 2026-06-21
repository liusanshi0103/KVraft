#pragma once
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <vector>
#include <chrono>
#include <string>
#include <utility>
#include "raft_rpc_util.h"
#include "apply_msg.h"
#include "raft.pb.h"
#include "util.h"
#include "blocking_queue.h"
#include "persister.h"

class Raft : public raft::RaftRpc {
 public:
  enum class Status {
    Follower,
    Candidate,
    Leader,
  };

 public:
  Raft();
  ~Raft();
  void Stop();
  void BecomeLeaderForTest();
  int LastLogIndexForTest();
  void ApplierTicker();
  void UpdateCommitIndex();
  void Persist();
  void ReadPersist(const std::string& data);

  bool PopApplyMsgForTest(ApplyMsg* msg, int timeout_ms);

  void Init(int me, const std::vector<std::pair<std::string, uint16_t>>& peer_addrs,std::shared_ptr<Persister> persister = nullptr);

  void GetState(int* term, bool* is_leader);

  void Start(const Op& command, int* new_log_index, int* new_log_term, bool* is_leader);

  void RequestVote(::google::protobuf::RpcController* controller,
                   const ::raft::RequestVoteArgs* request,
                   ::raft::RequestVoteReply* response,
                   ::google::protobuf::Closure* done) override;

  void AppendEntries(::google::protobuf::RpcController* controller,
                     const ::raft::AppendEntriesArgs* request,
                     ::raft::AppendEntriesReply* response,
                     ::google::protobuf::Closure* done) override;

 private:
 int LogTermAt(int log_index) const;
  void ElectionTicker();
  void DoElection();
  void RequestVoteImpl(const ::raft::RequestVoteArgs* args,
                       ::raft::RequestVoteReply* reply);

  void AppendEntriesImpl(const ::raft::AppendEntriesArgs* args,
                         ::raft::AppendEntriesReply* reply);
  int LastLogIndex() const;
  int LastLogTerm() const;
  int NewLogIndex() const;
  bool UpToDate(int candidate_last_log_index, int candidate_last_log_term) const;

  void BecomeFollower(int term);
  void BecomeCandidate();
  void BecomeLeader();
  void HeartbeatTicker();
  void DoHeartbeat();
  

  std::string StatusName() const;
  

 private:
  std::mutex mutex_;

  int me_ = -1;
  int peer_count_ = 0;

  int current_term_ = 0;
  int voted_for_ = -1;

  std::vector<raft::LogEntry> logs_;

  int commit_index_ = 0;
  int last_applied_ = 0;

  Status status_ = Status::Follower;
  std::chrono::steady_clock::time_point last_reset_election_time_;

  std::vector<int> next_index_;
  std::vector<int> match_index_;
  std::atomic<bool> stopped_{false};
  std::thread election_thread_;
  std::vector<std::shared_ptr<RaftRpcUtil>> peers_;
  std::thread heartbeat_thread_;
  std::shared_ptr<BlockingQueue<ApplyMsg>> apply_chan_;
  std::thread applier_thread_;
  std::shared_ptr<Persister> persister_;
};