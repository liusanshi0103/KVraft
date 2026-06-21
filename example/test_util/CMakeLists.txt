add_library(test_util_lib
    kv_raft_cluster.cpp
)

target_include_directories(test_util_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(test_util_lib PUBLIC
    kv_lib
    raft_lib
    rpc_lib
    common_lib
    protobuf
)