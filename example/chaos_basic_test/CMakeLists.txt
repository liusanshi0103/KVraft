add_executable(chaos_basic_test main.cpp)

target_link_libraries(chaos_basic_test PRIVATE
    clerk_lib
    test_util_lib
)