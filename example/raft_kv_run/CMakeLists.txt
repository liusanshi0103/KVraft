add_executable(raft_kv_node node_main.cpp)

target_link_libraries(raft_kv_node
    kv_lib
    raft_lib
    rpc_lib
    common_lib
    protobuf
    pthread
)

add_executable(raft_kv_client client_main.cpp)

target_link_libraries(raft_kv_client
    clerk_lib
    rpc_lib
    common_lib
    protobuf
    pthread
)