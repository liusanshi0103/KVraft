add_executable(skiplist_kv_test main.cpp)

target_link_libraries(skiplist_kv_test
    test_util_lib
    clerk_lib
    kv_lib
    raft_lib
    rpc_lib
    common_lib
    protobuf
    pthread
)