#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "kv_raft_cluster.h"
#include "kv_service.pb.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

bool RpcPutAppend(const std::string& ip,
                  uint16_t port,
                  const std::string& key,
                  const std::string& value,
                  const std::string& op,
                  const std::string& client_id,
                  int request_id) {
  kvraft::KvRaftServiceRpc_Stub stub(new MprpcChannel(ip, port));

  kvraft::PutAppendArgs args;
  args.set_key(key);
  args.set_value(value);
  args.set_op(op);
  args.set_client_id(client_id);
  args.set_request_id(request_id);

  kvraft::PutAppendReply reply;
  MprpcController controller;

  stub.PutAppend(&controller, &args, &reply, nullptr);

  if (controller.Failed()) {
    return false;
  }

  return reply.err() == "OK";
}

bool PutAppendToCluster(const std::vector<std::pair<std::string, uint16_t>>& addrs,
                        const std::string& key,
                        const std::string& value,
                        const std::string& op,
                        const std::string& client_id,
                        int request_id) {
  for (int i = 0; i < 3; ++i) {
    if (RpcPutAppend(addrs[i].first,
                     addrs[i].second,
                     key,
                     value,
                     op,
                     client_id,
                     request_id)) {
      return true;
    }
  }

  return false;
}

bool RpcGet(const std::string& ip,
            uint16_t port,
            const std::string& key,
            const std::string& client_id,
            int request_id,
            std::string* value) {
  kvraft::KvRaftServiceRpc_Stub stub(new MprpcChannel(ip, port));

  kvraft::GetArgs args;
  args.set_key(key);
  args.set_client_id(client_id);
  args.set_request_id(request_id);

  kvraft::GetReply reply;
  MprpcController controller;

  stub.Get(&controller, &args, &reply, nullptr);

  if (controller.Failed()) {
    return false;
  }

  if (reply.err() != "OK") {
    return false;
  }

  *value = reply.value();
  return true;
}

bool GetFromCluster(const std::vector<std::pair<std::string, uint16_t>>& addrs,
                    const std::string& key,
                    const std::string& client_id,
                    int request_id,
                    std::string* value) {
  for (int i = 0; i < 3; ++i) {
    if (RpcGet(addrs[i].first,
               addrs[i].second,
               key,
               client_id,
               request_id,
               value)) {
      return true;
    }
  }

  return false;
}

int main() {
  KvRaftCluster cluster(3, 16000, 16100);
  cluster.Start();

  const auto& kv_addrs = cluster.KvAddrs();

  const std::string client_id = "dedup-client";

  bool put_ok = PutAppendToCluster(
      kv_addrs, "x", "hello", "Put", client_id, 1);

  bool append_ok_1 = PutAppendToCluster(
      kv_addrs, "x", " world", "Append", client_id, 2);

  bool append_ok_2 = PutAppendToCluster(
      kv_addrs, "x", " world", "Append", client_id, 2);

  std::string value;
  bool get_ok = GetFromCluster(
      kv_addrs, "x", client_id, 3, &value);

  std::cout << "put_ok=" << put_ok << std::endl;
  std::cout << "append_ok_1=" << append_ok_1 << std::endl;
  std::cout << "append_ok_2=" << append_ok_2 << std::endl;
  std::cout << "get_ok=" << get_ok << std::endl;
  std::cout << "value=" << value << std::endl;

  cluster.Stop();

  if (!put_ok || !append_ok_1 || !append_ok_2 || !get_ok) {
    std::cout << "dedup test failed: request failed" << std::endl;
    return 1;
  }

  if (value != "hello world") {
    std::cout << "dedup test failed: duplicate append executed" << std::endl;
    return 1;
  }

  std::cout << "dedup test passed" << std::endl;
  return 0;
}