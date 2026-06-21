add_executable(dedupTestMain main.cpp)

target_link_libraries(dedupTestMain
  test_util_lib
  kv_lib
  raft_lib
  rpc_lib
  common_lib
  protobuf
)