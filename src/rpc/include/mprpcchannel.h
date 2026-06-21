#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

#include <string>

class MprpcChannel : public google::protobuf::RpcChannel {
 public:
  MprpcChannel();
  MprpcChannel(std::string ip, uint16_t port);

  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done) override;

 private:
  int NewConnect(std::string* err_msg);

 private:
  std::string ip_;
  uint16_t port_;
};