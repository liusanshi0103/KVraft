#include <iostream>
#include <string>
#include <unordered_map>

#include "kv.pb.h"
#include "rpcprovider.h"

const std::string OK = "OK";
const std::string ErrNoKey = "ErrNoKey";

class KvService : public kv::KvServiceRpc {
 public:
  void Get(::google::protobuf::RpcController* controller,
           const ::kv::GetArgs* request,
           ::kv::GetReply* response,
           ::google::protobuf::Closure* done) override {
    std::string key = request->key();

    auto it = kv_db_.find(key);
    if (it == kv_db_.end()) {
      response->set_err(ErrNoKey);
      response->set_value("");
    } else {
      response->set_err(OK);
      response->set_value(it->second);
    }

    done->Run();
  }

  void PutAppend(::google::protobuf::RpcController* controller,
                 const ::kv::PutAppendArgs* request,
                 ::kv::PutAppendReply* response,
                 ::google::protobuf::Closure* done) override {
    std::string key = request->key();
    std::string value = request->value();
    std::string op = request->op();

    if (op == "Put") {
      kv_db_[key] = value;
      response->set_err(OK);
    } else if (op == "Append") {
      kv_db_[key] += value;
      response->set_err(OK);
    } else {
      response->set_err("ErrUnknownOp");
    }

    done->Run();
  }

 private:
  std::unordered_map<std::string, std::string> kv_db_;
};

int main() {
  RpcProvider provider;

  provider.NotifyService(new KvService());

  provider.Run("127.0.0.1", 7799);

  return 0;
}