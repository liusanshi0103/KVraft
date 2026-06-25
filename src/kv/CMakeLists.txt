add_library(kv_lib
    ${CMAKE_CURRENT_SOURCE_DIR}/kv_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/kv_service.pb.cc
)

target_include_directories(kv_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
)

target_link_libraries(kv_lib PUBLIC
    raft_lib
    rpc_lib
    common_lib
    protobuf
    skiplist_lib
)