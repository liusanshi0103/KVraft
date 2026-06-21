add_executable(raftSkeletonMain main.cpp)

target_link_libraries(raftSkeletonMain
  raft_lib
  rpc_lib
  common_lib
  protobuf
)