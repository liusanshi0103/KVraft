#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

#include <string>
#include <unordered_map>

class RpcProvider {
 public:
  void NotifyService(google::protobuf::Service* service);

  void Run(const std::string& ip, uint16_t port);

 private:
  struct ServiceInfo {
    google::protobuf::Service* service;
    std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> method_map;
  };

 private:
  void OnMessage(int conn_fd, const std::string& recv_buf);
  void SendRpcResponse(int conn_fd, google::protobuf::Message* response);

 private:
  std::unordered_map<std::string, ServiceInfo> service_map_;
};