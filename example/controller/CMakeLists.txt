add_executable(controllerMain main.cpp)

target_link_libraries(controllerMain
  rpc_lib
  protobuf
)