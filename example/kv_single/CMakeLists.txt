include_directories(${PROJECT_SOURCE_DIR}/example/kv_single)

add_executable(kvServer
  kv_server.cpp
  kv.pb.cc
)

target_link_libraries(kvServer
  rpc_lib
  protobuf
)

add_executable(kvClient
  kv_client.cpp
  kv.pb.cc
)

target_link_libraries(kvClient
  rpc_lib
  protobuf
)