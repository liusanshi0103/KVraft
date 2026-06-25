cmake_minimum_required(VERSION 3.22)
project(MyKVRaft)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/src/common/include)
include_directories(${PROJECT_SOURCE_DIR}/src/rpc/include)
include_directories(${PROJECT_SOURCE_DIR}/src/rpc/proto)
include_directories(${PROJECT_SOURCE_DIR}/src/raft/include)
include_directories(${PROJECT_SOURCE_DIR}/src/raft/proto)
include_directories(${PROJECT_SOURCE_DIR}/src/skipList/include)
add_subdirectory(src/skipList)

add_subdirectory(src/common)
add_subdirectory(src/rpc)
add_subdirectory(src/raft)
add_subdirectory(src/kv)
add_subdirectory(src/clerk)
add_subdirectory(example)