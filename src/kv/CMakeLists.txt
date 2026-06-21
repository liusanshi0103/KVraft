add_library(kv
    kv_server.cpp
)

target_include_directories(kv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kv PUBLIC
    raft
    common
)