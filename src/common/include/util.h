#pragma once

#include <string>
#include <sstream>
#include <chrono>
#include <random>
#include <thread>
std::string helloCommon();
int RandomElectionTimeoutMs();

void SleepMs(int ms);
struct Op {
  std::string operation;  // "Get", "Put", "Append"
  std::string key;
  std::string value;
  std::string client_id;
  int request_id = 0;

   std::string Serialize() const {
    return operation + "\n" +
           key + "\n" +
           value + "\n" +
           client_id + "\n" +
           std::to_string(request_id);
  }

  static Op Deserialize(const std::string& data) {
    Op op;
    std::stringstream ss(data);

    std::getline(ss, op.operation);
    std::getline(ss, op.key);
    std::getline(ss, op.value);
    std::getline(ss, op.client_id);

    std::string request_id_str;
    std::getline(ss, request_id_str);
    op.request_id = std::stoi(request_id_str);

    return op;
  }
  std::string DebugString() const {
    std::stringstream ss;
    ss << "Op{operation=" << operation
       << ", key=" << key
       << ", value=" << value
       << ", client_id=" << client_id
       << ", request_id=" << request_id
       << "}";
    return ss.str();
  }
};