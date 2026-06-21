add_executable(fullRestartTestMain main.cpp)

target_link_libraries(fullRestartTestMain
  test_util_lib
  clerk_lib
  kv_lib
  raft_lib
  rpc_lib
  common_lib
  protobuf
)