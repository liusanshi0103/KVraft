#include <iostream>
#include <string>
#include "friend.pb.h"

int main() {
  fixbug::GetFriendsListRequest request;
  request.set_userid(1000);

  std::string requestStr;
  request.SerializeToString(&requestStr);

  fixbug::GetFriendsListRequest request2;
  request2.ParseFromString(requestStr);

  std::cout << "userid: " << request2.userid() << std::endl;

  fixbug::GetFriendsListResponse response;
  response.mutable_result()->set_errcode(0);
  response.mutable_result()->set_errmsg("");

  response.add_friends("gao yang");
  response.add_friends("liu hong");
  response.add_friends("wang shuo");

  std::string responseStr;
  response.SerializeToString(&responseStr);

  fixbug::GetFriendsListResponse response2;
  response2.ParseFromString(responseStr);

  std::cout << "errcode: " << response2.result().errcode() << std::endl;
  std::cout << "errmsg: " << response2.result().errmsg() << std::endl;

  for (int i = 0; i < response2.friends_size(); ++i) {
    std::cout << "friend " << i << ": " << response2.friends(i) << std::endl;
  }

  return 0;
}