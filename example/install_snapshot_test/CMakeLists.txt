add_executable(install_snapshot_test main.cpp)

target_link_libraries(install_snapshot_test PRIVATE
    clerk_lib
    test_util_lib
)