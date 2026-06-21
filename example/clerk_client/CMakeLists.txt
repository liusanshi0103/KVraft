add_executable(clerkClientMain main.cpp)

target_link_libraries(clerkClientMain
  clerk_lib
  kv_lib
  raft_lib
  rpc_lib
  common_lib
  protobuf
)