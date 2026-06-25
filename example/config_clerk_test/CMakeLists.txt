add_executable(config_clerk_test main.cpp)

target_link_libraries(config_clerk_test
    test_util_lib
    clerk_lib
    kv_lib
    raft_lib
    rpc_lib
    common_lib
    protobuf
    pthread
)