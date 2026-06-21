include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/proto)

add_library(raft_lib
  raft.cpp
  raft_rpc_util.cpp
  proto/raft.pb.cc
)

target_link_libraries(raft_lib
  common_lib
  rpc_lib
  protobuf
)