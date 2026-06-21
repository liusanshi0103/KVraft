#include <iostream>
#include <string>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "rpcheader.pb.h"
#include "friend.pb.h"

int main() {
  // 1. 构造业务请求 args
  fixbug::GetFriendsListRequest request;
  request.set_userid(1000);

  std::string argsStr;
  request.SerializeToString(&argsStr);

  // 2. 构造 RPC 内部请求头
  RPC::RpcHeader header;
  header.set_service_name("FiendServiceRpc");
  header.set_method_name("GetFriendsList");
  header.set_args_size(argsStr.size());

  std::string headerStr;
  header.SerializeToString(&headerStr);

  // 3. 拼接最终 RPC 请求包：header_size + header + args
  std::string sendRpcStr;

  {
    google::protobuf::io::StringOutputStream stringOutput(&sendRpcStr);
    google::protobuf::io::CodedOutputStream codedOutput(&stringOutput);

    codedOutput.WriteVarint32(static_cast<uint32_t>(headerStr.size()));
    codedOutput.WriteString(headerStr);
  }

  sendRpcStr += argsStr;

  std::cout << "send rpc total size: " << sendRpcStr.size() << std::endl;

  // 4. 模拟服务端拆包
  google::protobuf::io::ArrayInputStream arrayInput(sendRpcStr.data(), sendRpcStr.size());
  google::protobuf::io::CodedInputStream codedInput(&arrayInput);

  uint32_t headerSize = 0;
  codedInput.ReadVarint32(&headerSize);

  std::string recvHeaderStr;
  codedInput.ReadString(&recvHeaderStr, headerSize);

  RPC::RpcHeader recvHeader;
  recvHeader.ParseFromString(recvHeaderStr);

  std::cout << "service_name: " << recvHeader.service_name() << std::endl;
  std::cout << "method_name: " << recvHeader.method_name() << std::endl;
  std::cout << "args_size: " << recvHeader.args_size() << std::endl;

  std::string recvArgsStr;
  codedInput.ReadString(&recvArgsStr, recvHeader.args_size());

  fixbug::GetFriendsListRequest recvRequest;
  recvRequest.ParseFromString(recvArgsStr);

  std::cout << "userid: " << recvRequest.userid() << std::endl;

  return 0;
}