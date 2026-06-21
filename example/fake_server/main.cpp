#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "friend.pb.h"

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    std::cerr << "socket failed" << std::endl;
    return 1;
  }

  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7788);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
    std::cerr << "bind failed" << std::endl;
    close(listen_fd);
    return 1;
  }

  if (listen(listen_fd, 10) == -1) {
    std::cerr << "listen failed" << std::endl;
    close(listen_fd);
    return 1;
  }

  std::cout << "fake server listen on 127.0.0.1:7788" << std::endl;

  int conn_fd = accept(listen_fd, nullptr, nullptr);
  if (conn_fd == -1) {
    std::cerr << "accept failed" << std::endl;
    close(listen_fd);
    return 1;
  }

  char recv_buf[4096] = {0};
  int recv_size = recv(conn_fd, recv_buf, sizeof(recv_buf), 0);
  std::cout << "fake server recv size: " << recv_size << std::endl;

  fixbug::GetFriendsListResponse response;
  response.mutable_result()->set_errcode(0);
  response.mutable_result()->set_errmsg("");
  response.add_friends("gao yang");
  response.add_friends("liu hong");
  response.add_friends("wang shuo");

  std::string response_str;
  response.SerializeToString(&response_str);

  send(conn_fd, response_str.data(), response_str.size(), 0);

  close(conn_fd);
  close(listen_fd);

  return 0;
}