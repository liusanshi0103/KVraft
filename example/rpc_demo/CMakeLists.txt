include_directories(${PROJECT_SOURCE_DIR}/example/protobuf)

add_executable(provider
  provider.cpp
  ${PROJECT_SOURCE_DIR}/example/protobuf/friend.pb.cc
)

target_link_libraries(provider
  rpc_lib
  protobuf
)

add_executable(consumer
  consumer.cpp
  ${PROJECT_SOURCE_DIR}/example/protobuf/friend.pb.cc
)

target_link_libraries(consumer
  rpc_lib
  protobuf
)