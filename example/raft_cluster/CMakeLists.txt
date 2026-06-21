add_executable(raftClusterMain main.cpp)

target_link_libraries(raftClusterMain
  raft_lib
  rpc_lib
  common_lib
  protobuf
)