#include "mprpcchannel.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "mprpccontroller.h"
#include "rpcheader.pb.h"
MprpcChannel::MprpcChannel()=default;
MprpcChannel::MprpcChannel(std::string ip, uint16_t port)
    : ip_(std::move(ip)), port_(port) {}

int MprpcChannel::NewConnect(std::string* err_msg) {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    *err_msg = "create socket failed, errno: " + std::to_string(errno);
    return -1;
  }

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_);
  server_addr.sin_addr.s_addr = inet_addr(ip_.c_str());

  if (connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
    *err_msg = "connect failed, errno: " + std::to_string(errno);
    close(client_fd);
    return false;
  }

  return client_fd;
}

void MprpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done) {
  const google::protobuf::ServiceDescriptor* service_desc = method->service();

  std::string service_name = service_desc->name();
  std::string method_name = method->name();

  std::string args_str;
  if (!request->SerializeToString(&args_str)) {
    controller->SetFailed("serialize request failed");
    return;
  }

  RPC::RpcHeader rpc_header;
  rpc_header.set_service_name(service_name);
  rpc_header.set_method_name(method_name);
  rpc_header.set_args_size(args_str.size());

  std::string rpc_header_str;
  if (!rpc_header.SerializeToString(&rpc_header_str)) {
    controller->SetFailed("serialize rpc header failed");
    return;
  }

  std::string send_rpc_str;
  {
    google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
    google::protobuf::io::CodedOutputStream coded_output(&string_output);

    coded_output.WriteVarint32(static_cast<uint32_t>(rpc_header_str.size()));
    coded_output.WriteString(rpc_header_str);
  }

  send_rpc_str += args_str;

  std::string err_msg;
  int client_fd = NewConnect(&err_msg);
  if (client_fd == -1) {
    controller->SetFailed(err_msg);
    return;
  }

  int send_size = send(client_fd, send_rpc_str.data(), send_rpc_str.size(), 0);
  if (send_size == -1) {
    controller->SetFailed("send failed, errno: " + std::to_string(errno));
    close(client_fd);
    return;
  }

  char recv_buf[4096] = {0};
  int recv_size = recv(client_fd, recv_buf, sizeof(recv_buf), 0);
  if (recv_size == -1) {
    controller->SetFailed("recv failed, errno: " + std::to_string(errno));
    close(client_fd);
    return;
  }

  if (!response->ParseFromArray(recv_buf, recv_size)) {
    controller->SetFailed("parse response failed");
    close(client_fd);
    return;
  }

  close(client_fd);
}