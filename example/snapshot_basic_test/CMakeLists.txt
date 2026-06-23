add_executable(snapshot_basic_test main.cpp)

target_link_libraries(snapshot_basic_test PRIVATE
    clerk_lib
    test_util_lib
)