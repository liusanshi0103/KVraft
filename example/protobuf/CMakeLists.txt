add_executable(protobufMain main.cpp friend.pb.cc)

target_link_libraries(protobufMain protobuf)