#include "rpcprovider.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <cstring>
#include <iostream>
#include <string>

#include "rpcheader.pb.h"

void RpcProvider::NotifyService(google::protobuf::Service* service) {
  const google::protobuf::ServiceDescriptor* service_desc = service->GetDescriptor();

  std::string service_name = service_desc->name();

  ServiceInfo service_info;
  service_info.service = service;

  int method_count = service_desc->method_count();
  for (int i = 0; i < method_count; ++i) {
    const google::protobuf::MethodDescriptor* method_desc = service_desc->method(i);
    service_info.method_map.emplace(method_desc->name(), method_desc);
  }

  service_map_.emplace(service_name, service_info);

  std::cout << "register service: " << service_name << std::endl;
}

void RpcProvider::Run(const std::string& ip, uint16_t port) {
  ip_ = ip;
  port_ = port;
  stopped_ = false;
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ == -1) {
    std::cerr << "socket failed" << std::endl;
    return;
  }

  int opt = 1;
  setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip.c_str());

  if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    std::cerr << "bind failed" << std::endl;
    close(listen_fd_);
    return;
  }

  if (listen(listen_fd_, 10) == -1) {
    std::cerr << "listen failed" << std::endl;
    close(listen_fd_);
    return;
  }

  std::cout << "RpcProvider listen on " << ip << ":" << port << std::endl;

  while (!stopped_) {
    int conn_fd = accept(listen_fd_, nullptr, nullptr);
     if (conn_fd < 0) {
    if (stopped_) {
      break;
    }
    continue;
  }
  if (stopped_) {
  close(conn_fd);
  break;
}
    char recv_buf[4096] = {0};
    int recv_size = recv(conn_fd, recv_buf, sizeof(recv_buf), 0);
    if (recv_size <= 0) {
      close(conn_fd);
      continue;
    }

    OnMessage(conn_fd, std::string(recv_buf, recv_size));
  }

  close(listen_fd_);
}

void RpcProvider::OnMessage(int conn_fd, const std::string& recv_buf) {
  google::protobuf::io::ArrayInputStream array_input(recv_buf.data(), recv_buf.size());
  google::protobuf::io::CodedInputStream coded_input(&array_input);

  uint32_t header_size = 0;
  if (!coded_input.ReadVarint32(&header_size)) {
    std::cerr << "read header size failed" << std::endl;
    close(conn_fd);
    return;
  }

  std::string rpc_header_str;
  if (!coded_input.ReadString(&rpc_header_str, header_size)) {
    std::cerr << "read rpc header failed" << std::endl;
    close(conn_fd);
    return;
  }

  RPC::RpcHeader rpc_header;
  if (!rpc_header.ParseFromString(rpc_header_str)) {
    std::cerr << "parse rpc header failed" << std::endl;
    close(conn_fd);
    return;
  }

  std::string service_name = rpc_header.service_name();
  std::string method_name = rpc_header.method_name();
  uint32_t args_size = rpc_header.args_size();

  std::string args_str;
  if (!coded_input.ReadString(&args_str, args_size)) {
    std::cerr << "read args failed" << std::endl;
    close(conn_fd);
    return;
  }

  auto service_it = service_map_.find(service_name);
  if (service_it == service_map_.end()) {
    std::cerr << "service not found: " << service_name << std::endl;
    close(conn_fd);
    return;
  }

  auto method_it = service_it->second.method_map.find(method_name);
  if (method_it == service_it->second.method_map.end()) {
    std::cerr << "method not found: " << method_name << std::endl;
    close(conn_fd);
    return;
  }

  google::protobuf::Service* service = service_it->second.service;
  const google::protobuf::MethodDescriptor* method = method_it->second;

  google::protobuf::Message* request = service->GetRequestPrototype(method).New();
  if (!request->ParseFromString(args_str)) {
    std::cerr << "parse request failed" << std::endl;
    delete request;
    close(conn_fd);
    return;
  }

  google::protobuf::Message* response = service->GetResponsePrototype(method).New();

  google::protobuf::Closure* done =
      google::protobuf::NewCallback<RpcProvider, int, google::protobuf::Message*>(
          this, &RpcProvider::SendRpcResponse, conn_fd, response);

  service->CallMethod(method, nullptr, request, response, done);

  delete request;
}

void RpcProvider::SendRpcResponse(int conn_fd, google::protobuf::Message* response) {
  std::string response_str;
  if (!response->SerializeToString(&response_str)) {
    std::cerr << "serialize response failed" << std::endl;
    delete response;
    close(conn_fd);
    return;
  }

  send(conn_fd, response_str.data(), response_str.size(), 0);

  delete response;
  close(conn_fd);
}
void RpcProvider::Stop() {
  stopped_ = true;

  if (listen_fd_ >= 0) {
    shutdown(listen_fd_, SHUT_RDWR);
    close(listen_fd_);
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd >= 0 && !ip_.empty() && port_ != 0) {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr);

    connect(sockfd,
            reinterpret_cast<sockaddr*>(&server_addr),
            sizeof(server_addr));

    close(sockfd);
  }

  listen_fd_ = -1;
}