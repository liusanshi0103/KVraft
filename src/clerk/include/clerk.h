#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "kv_service.pb.h"

class Clerk {
 public:
  explicit Clerk(const std::vector<std::pair<std::string, uint16_t>>& servers);

  void Put(const std::string& key, const std::string& value);
  void Append(const std::string& key, const std::string& value);
  std::string Get(const std::string& key);
  void SetLeaderForTest(int leader_id);

 private:
  bool PutAppendInternal(const std::string& key,
                         const std::string& value,
                         const std::string& op);

  int NextRequestId();
  

 private:
  std::vector<std::pair<std::string, uint16_t>> servers_;
  int leader_id_;
  std::string client_id_;
  int request_id_;
};