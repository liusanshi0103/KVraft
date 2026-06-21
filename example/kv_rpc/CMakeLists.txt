
add_executable(kvRpcMain main.cpp)

target_link_libraries(kvRpcMain
  kv_lib
  raft_lib
  rpc_lib
  common_lib
  protobuf
)