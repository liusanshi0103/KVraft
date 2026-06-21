add_library(clerk_lib
    clerk.cpp
)

target_include_directories(clerk_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(clerk_lib PUBLIC
    kv_lib
    rpc_lib
    common_lib
    protobuf
)