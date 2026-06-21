add_executable(persisterTestMain main.cpp)

target_link_libraries(persisterTestMain
  raft_lib
  rpc_lib
  common_lib
  protobuf
)