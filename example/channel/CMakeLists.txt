include_directories(${PROJECT_SOURCE_DIR}/example/protobuf)

add_executable(channelMain
  main.cpp
  ${PROJECT_SOURCE_DIR}/example/protobuf/friend.pb.cc
)

target_link_libraries(channelMain
  rpc_lib
  protobuf
)