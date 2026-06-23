add_executable(snapshot_restart_test main.cpp)

target_link_libraries(snapshot_restart_test PRIVATE
    clerk_lib
    test_util_lib
)