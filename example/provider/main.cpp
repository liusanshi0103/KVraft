#include <iostream>
#include <string>
#include <vector>

#include "friend.pb.h"
#include "rpcprovider.h"

class FriendService : public fixbug::FiendServiceRpc {
 public:
  std::vector<std::string> GetFriendsList(uint32_t userid) {
    std::cout << "local GetFriendsList, userid: " << userid << std::endl;

    return {
        "gao yang",
        "liu hong",
        "wang shuo",
    };
  }

  void GetFriendsList(::google::protobuf::RpcController* controller,
                      const ::fixbug::GetFriendsListRequest* request,
                      ::fixbug::GetFriendsListResponse* response,
                      ::google::protobuf::Closure* done) override {
    uint32_t userid = request->userid();

    std::vector<std::string> friends = GetFriendsList(userid);

    response->mutable_result()->set_errcode(0);
    response->mutable_result()->set_errmsg("");

    for (const auto& name : friends) {
      response->add_friends(name);
    }

    done->Run();
  }
};

int main() {
  RpcProvider provider;

  provider.NotifyService(new FriendService());

  provider.Run("127.0.0.1", 7788);

  return 0;
}