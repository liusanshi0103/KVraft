#include "raft.h"

#include <iostream>

Raft::Raft() = default;
Raft::~Raft() {
  Stop();
}

void Raft::Stop() {
  stopped_ = true;

  if (election_thread_.joinable()) {
    election_thread_.join();
  }
  if (heartbeat_thread_.joinable()) {
    heartbeat_thread_.join();
  }
  if (applier_thread_.joinable()) {
  applier_thread_.join();
}
}
int Raft::LogTermAt(int log_index) const {
  if (log_index == 0) {
    return 0;
  }

  if (log_index < 0 || log_index > LastLogIndex()) {
    return -1;
  }

  return logs_[log_index - 1].log_term();
}

int Raft::LastLogIndexForTest() {
  std::lock_guard<std::mutex> lock(mutex_);
  return LastLogIndex();
}

void Raft::Init(int me, const std::vector<std::pair<std::string, uint16_t>>& peer_addrs) {
 { std::lock_guard<std::mutex> lock(mutex_);

  me_ = me;
  peer_count_ = static_cast<int>(peer_addrs.size());;
  current_term_ = 0;
  voted_for_ = -1;
  commit_index_ = 0;
  last_applied_ = 0;
  status_ = Status::Follower;
  last_reset_election_time_ = std::chrono::steady_clock::now();

  next_index_.assign(peer_count_, 1);
  match_index_.assign(peer_count_, 0);
  peers_.clear();
  for (int i = 0; i < peer_count_; ++i) {
      if (i == me_) {
        peers_.push_back(nullptr);
      } else {
        peers_.push_back(std::make_shared<RaftRpcUtil>(
            peer_addrs[i].first,
            peer_addrs[i].second));
      }
    }
  std::cout << "Raft node " << me_ << " init, peer_count=" << peer_count_ << std::endl;
 }
  stopped_ = false;
  election_thread_ = std::thread(&Raft::ElectionTicker, this);
  heartbeat_thread_ = std::thread(&Raft::HeartbeatTicker, this);
  apply_chan_ = std::make_shared<BlockingQueue<ApplyMsg>>();
  applier_thread_ = std::thread(&Raft::ApplierTicker, this);
}
void Raft::ElectionTicker() {
  while (!stopped_) {
    int timeout_ms = RandomElectionTimeoutMs();

    SleepMs(timeout_ms);

    bool should_start_election = false;

    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (status_ == Status::Leader) {
        continue;
      }

      auto now = std::chrono::steady_clock::now();
      auto elapsed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              now - last_reset_election_time_)
              .count();

      if (elapsed_ms >= timeout_ms) {
        should_start_election = true;
      }
    }

    if (should_start_election) {
      DoElection();
    }
  }
}
void Raft::DoElection() {
  int term = 0;
  int candidate_id = 0;
  int last_log_index = 0;
  int last_log_term = 0;
  int peer_count = 0;

  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (status_ == Status::Leader) {
      return;
    }

    BecomeCandidate();

    term = current_term_;
    candidate_id = me_;
    last_log_index = LastLogIndex();
    last_log_term = LastLogTerm();
    peer_count = peer_count_;

    std::cout << "node " << me_
              << " start election, term=" << current_term_
              << ", last_log_index=" << last_log_index
              << ", last_log_term=" << last_log_term
              << std::endl;
  }

  auto vote_count = std::make_shared<std::atomic<int>>(1);

  for (int i = 0; i < peer_count; ++i) {
    if (i == candidate_id) {
      continue;
    }

    std::thread([this, i, term, candidate_id, last_log_index, last_log_term, vote_count]() {
      raft::RequestVoteArgs args;
      args.set_term(term);
      args.set_candidate_id(candidate_id);
      args.set_last_log_index(last_log_index);
      args.set_last_log_term(last_log_term);

      raft::RequestVoteReply reply;

      bool ok = peers_[i]->RequestVote(&args, &reply);
      if (!ok) {
        return;
      }

      std::lock_guard<std::mutex> lock(mutex_);

      if (reply.term() > current_term_) {
        BecomeFollower(reply.term());
        return;
      }

      if (status_ != Status::Candidate) {
        return;
      }

      if (term != current_term_) {
        return;
      }

      if (!reply.vote_granted()) {
        return;
      }

      int new_vote_count = ++(*vote_count);

      std::cout << "node " << me_
                << " got vote from " << i
                << ", vote_count=" << new_vote_count
                << ", term=" << current_term_
                << std::endl;

      if (new_vote_count >= peer_count_ / 2 + 1 &&
          status_ == Status::Candidate) {
        BecomeLeader();
      }
    }).detach();
  }
}
void Raft::HeartbeatTicker() {
  while (!stopped_) {
    SleepMs(80);
    DoHeartbeat();
  }
}
void Raft::DoHeartbeat() {
  struct AppendTask {
    int server;
    raft::AppendEntriesArgs args;
  };

  std::vector<AppendTask> tasks;

  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (status_ != Status::Leader) {
      return;
    }

    for (int i = 0; i < peer_count_; ++i) {
      if (i == me_) {
        continue;
      }

      int next_index = next_index_[i];
      int prev_log_index = next_index - 1;
      int prev_log_term = LogTermAt(prev_log_index);

      raft::AppendEntriesArgs args;
      args.set_term(current_term_);
      args.set_leader_id(me_);
      args.set_prev_log_index(prev_log_index);
      args.set_prev_log_term(prev_log_term);
      args.set_leader_commit(commit_index_);

      for (int log_index = next_index; log_index <= LastLogIndex(); ++log_index) {
        *args.add_entries() = logs_[log_index - 1];
      }

      tasks.push_back({i, args});
    }
  }

  for (auto task : tasks) {
    std::thread([this, task]() mutable {
      raft::AppendEntriesReply reply;

      bool ok = peers_[task.server]->AppendEntries(&task.args, &reply);
      if (!ok) {
        return;
      }

      std::lock_guard<std::mutex> lock(mutex_);

      if (reply.term() > current_term_) {
        BecomeFollower(reply.term());
        return;
      }

      if (status_ != Status::Leader || task.args.term() != current_term_) {
        return;
      }

      if (reply.success()) {
        int replicated_index =
            task.args.prev_log_index() + task.args.entries_size();

        match_index_[task.server] =
            std::max(match_index_[task.server], replicated_index);

        next_index_[task.server] = match_index_[task.server] + 1;
        UpdateCommitIndex();
      } else {
        next_index_[task.server] =
            std::max(1, reply.update_next_index());
      }
    }).detach();
  }
}
bool Raft::PopApplyMsgForTest(ApplyMsg* msg, int timeout_ms) {
  return apply_chan_->Pop(msg, timeout_ms);
}
void Raft::ApplierTicker() {
  while (!stopped_) {
    std::vector<ApplyMsg> apply_msgs;

    {
      std::lock_guard<std::mutex> lock(mutex_);

      while (last_applied_ < commit_index_) {
        ++last_applied_;

        const auto& log = logs_[last_applied_ - 1];

        ApplyMsg msg;
        msg.command_valid = true;
        msg.command = log.command();
        msg.command_index = last_applied_;

        apply_msgs.push_back(msg);
      }
    }

    for (const auto& msg : apply_msgs) {
      apply_chan_->Push(msg);
    }

    SleepMs(10);
  }
}
void Raft::GetState(int* term, bool* is_leader) {
  std::lock_guard<std::mutex> lock(mutex_);

  *term = current_term_;
  *is_leader = (status_ == Status::Leader);
}

void Raft::Start(const Op& command, int* new_log_index, int* new_log_term, bool* is_leader) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (status_ != Status::Leader) {
    *new_log_index = -1;
    *new_log_term = -1;
    *is_leader = false;
    return;
  }

  raft::LogEntry entry;
  entry.set_command(command.Serialize());
  entry.set_log_term(current_term_);
  entry.set_log_index(NewLogIndex());

  logs_.push_back(entry);

  *new_log_index = entry.log_index();
  *new_log_term = entry.log_term();
  *is_leader = true;

  std::cout << "leader " << me_
            << " append local log, index=" << entry.log_index()
            << ", term=" << entry.log_term()
            << ", command=" << command.DebugString()
            << std::endl;
}

void Raft::RequestVoteImpl(const ::raft::RequestVoteArgs* args,
                           ::raft::RequestVoteReply* reply) {
  std::lock_guard<std::mutex> lock(mutex_);

  reply->set_term(current_term_);
  reply->set_vote_granted(false);

  if(args->term()<current_term_){
    return;
  }
  if(args->term()>current_term_){
    BecomeFollower(args->term());
  }
  bool can_vote = (voted_for_==-1||voted_for_==args->candidate_id());
  bool log_ok = UpToDate(args->last_log_index(),args->last_log_term());
   if (can_vote && log_ok) {
    voted_for_ = args->candidate_id();
    last_reset_election_time_ = std::chrono::steady_clock::now();

    reply->set_term(current_term_);
    reply->set_vote_granted(true);
        std::cout << "node " << me_
              << " vote for " << args->candidate_id()
              << ", term=" << current_term_
              << std::endl;
    return;
  }

}

void Raft::AppendEntriesImpl(const ::raft::AppendEntriesArgs* args,
                             ::raft::AppendEntriesReply* reply) {
  std::lock_guard<std::mutex> lock(mutex_);

  reply->set_term(current_term_);
  reply->set_success(false);
  if (args->term() < current_term_) {
    return;
  }

  if (args->term() > current_term_) {
    BecomeFollower(args->term());
  } else {
    status_ = Status::Follower;
  }

  last_reset_election_time_ = std::chrono::steady_clock::now();
 if (args->prev_log_index() > LastLogIndex()) {
    reply->set_term(current_term_);
    reply->set_success(false);
    reply->set_update_next_index(LastLogIndex() + 1);
    return;
  }

  if (LogTermAt(args->prev_log_index()) != args->prev_log_term()) {
    reply->set_term(current_term_);
    reply->set_success(false);
    reply->set_update_next_index(std::max(1, args->prev_log_index()));
    return;
  }

  for (int i = 0; i < args->entries_size(); ++i) {
    const raft::LogEntry& entry = args->entries(i);
    int log_index = entry.log_index();

    if (log_index <= LastLogIndex()) {
      if (LogTermAt(log_index) != entry.log_term()) {
        logs_.resize(log_index - 1);
        logs_.push_back(entry);
      }
    } else {
      logs_.push_back(entry);
    }
  }

  if (args->leader_commit() > commit_index_) {
    commit_index_ = std::min(args->leader_commit(), LastLogIndex());
  }
  reply->set_term(current_term_);
  reply->set_success(true);
  reply->set_update_next_index(LastLogIndex() + 1);

    std::cout << "node " << me_
            << " accept AppendEntries from leader " << args->leader_id()
            << ", entries=" << args->entries_size()
            << ", last_log_index=" << LastLogIndex()
            << std::endl;
}

void Raft::RequestVote(::google::protobuf::RpcController* controller,
                       const ::raft::RequestVoteArgs* request,
                       ::raft::RequestVoteReply* response,
                       ::google::protobuf::Closure* done) {
  RequestVoteImpl(request, response);
  done->Run();
}

void Raft::AppendEntries(::google::protobuf::RpcController* controller,
                         const ::raft::AppendEntriesArgs* request,
                         ::raft::AppendEntriesReply* response,
                         ::google::protobuf::Closure* done) {
  AppendEntriesImpl(request, response);
  done->Run();
}
int Raft::LastLogIndex() const {
  if (logs_.empty()) {
    return 0;
  }
  return logs_.back().log_index();
}

int Raft::LastLogTerm() const {
  if (logs_.empty()) {
    return 0;
  }
  return logs_.back().log_term();
}

int Raft::NewLogIndex() const {
  return LastLogIndex() + 1;
}

bool Raft::UpToDate(int candidate_last_log_index,
                    int candidate_last_log_term) const {
  int my_last_log_term = LastLogTerm();
  int my_last_log_index = LastLogIndex();

  if (candidate_last_log_term != my_last_log_term) {
    return candidate_last_log_term > my_last_log_term;
  }

  return candidate_last_log_index >= my_last_log_index;
}

void Raft::UpdateCommitIndex() {
  for (int n = LastLogIndex(); n > commit_index_; --n) {
    if (LogTermAt(n) != current_term_) {
      continue;
    }

    int count = 1;  // leader 自己

    for (int i = 0; i < peer_count_; ++i) {
      if (i == me_) {
        continue;
      }

      if (match_index_[i] >= n) {
        ++count;
      }
    }

    if (count > peer_count_ / 2) {
      commit_index_ = n;
      break;
    }
  }
}
void Raft::BecomeFollower(int term) {
  status_ = Status::Follower;
  current_term_ = term;
  voted_for_ = -1;
  last_reset_election_time_ = std::chrono::steady_clock::now();

  std::cout << "node " << me_ << " become Follower, term="
            << current_term_ << std::endl;
}

void Raft::BecomeCandidate() {
  status_ = Status::Candidate;
  current_term_ += 1;
  voted_for_ = me_;
  last_reset_election_time_ = std::chrono::steady_clock::now();

  std::cout << "node " << me_ << " become Candidate, term="
            << current_term_ << std::endl;
}

void Raft::BecomeLeader() {
  status_ = Status::Leader;

  int next_index = LastLogIndex() + 1;
  next_index_.assign(peer_count_, next_index);
  match_index_.assign(peer_count_, 0);

  std::cout << "node " << me_ << " become Leader, term="
            << current_term_ << std::endl;
}
std::string Raft::StatusName() const {
  switch (status_) {
    case Status::Follower:
      return "Follower";
    case Status::Candidate:
      return "Candidate";
    case Status::Leader:
      return "Leader";
  }
  return "Unknown";
}