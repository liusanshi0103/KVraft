include_directories(${PROJECT_SOURCE_DIR}/example/protobuf)

add_executable(fakeServerMain
  main.cpp
  ${PROJECT_SOURCE_DIR}/example/protobuf/friend.pb.cc
)

target_link_libraries(fakeServerMain protobuf)