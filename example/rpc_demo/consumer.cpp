#include <iostream>

#include "friend.pb.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

int main() {
  fixbug::FiendServiceRpc_Stub stub(new MprpcChannel("127.0.0.1", 7788));

  fixbug::GetFriendsListRequest request;
  request.set_userid(1000);

  fixbug::GetFriendsListResponse response;

  MprpcController controller;

  stub.GetFriendsList(&controller, &request, &response, nullptr);

  if (controller.Failed()) {
    std::cout << "rpc failed: " << controller.ErrorText() << std::endl;
    return 1;
  }

  if (response.result().errcode() == 0) {
    std::cout << "rpc GetFriendsList response success" << std::endl;

    for (int i = 0; i < response.friends_size(); ++i) {
      std::cout << "friend " << i << ": " << response.friends(i) << std::endl;
    }
  } else {
    std::cout << "rpc GetFriendsList response error: "
              << response.result().errmsg() << std::endl;
  }

  return 0;
}