add_executable(clerkRetryTestMain main.cpp)

target_link_libraries(clerkRetryTestMain
  test_util_lib
  clerk_lib
  kv_lib
  raft_lib
  rpc_lib
  common_lib
  protobuf
)