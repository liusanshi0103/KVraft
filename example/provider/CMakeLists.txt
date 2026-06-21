include_directories(${PROJECT_SOURCE_DIR}/example/protobuf)

add_executable(providerMain
  main.cpp
  ${PROJECT_SOURCE_DIR}/example/protobuf/friend.pb.cc
)

target_link_libraries(providerMain
  rpc_lib
  protobuf
)