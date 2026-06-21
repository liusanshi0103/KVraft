#include <iostream>
#include <string>

#include "kv.pb.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

void Put(kv::KvServiceRpc_Stub& stub, const std::string& key, const std::string& value) {
  kv::PutAppendArgs args;
  args.set_key(key);
  args.set_value(value);
  args.set_op("Put");

  kv::PutAppendReply reply;
  MprpcController controller;

  stub.PutAppend(&controller, &args, &reply, nullptr);

  if (controller.Failed()) {
    std::cout << "Put failed: " << controller.ErrorText() << std::endl;
    return;
  }

  std::cout << "Put " << key << "=" << value << " " << reply.err() << std::endl;
}

void Append(kv::KvServiceRpc_Stub& stub, const std::string& key, const std::string& value) {
  kv::PutAppendArgs args;
  args.set_key(key);
  args.set_value(value);
  args.set_op("Append");

  kv::PutAppendReply reply;
  MprpcController controller;

  stub.PutAppend(&controller, &args, &reply, nullptr);

  if (controller.Failed()) {
    std::cout << "Append failed: " << controller.ErrorText() << std::endl;
    return;
  }

  std::cout << "Append " << key << "+=" << value << " " << reply.err() << std::endl;
}

std::string Get(kv::KvServiceRpc_Stub& stub, const std::string& key) {
  kv::GetArgs args;
  args.set_key(key);

  kv::GetReply reply;
  MprpcController controller;

  stub.Get(&controller, &args, &reply, nullptr);

  if (controller.Failed()) {
    std::cout << "Get failed: " << controller.ErrorText() << std::endl;
    return "";
  }

  if (reply.err() == "ErrNoKey") {
    return "";
  }

  return reply.value();
}

int main() {
  kv::KvServiceRpc_Stub stub(new MprpcChannel("127.0.0.1", 7799));

  Put(stub, "x", "hello");
  Append(stub, "x", " world");

  std::string value = Get(stub, "x");

  std::cout << "Get x: " << value << std::endl;

  return 0;
}