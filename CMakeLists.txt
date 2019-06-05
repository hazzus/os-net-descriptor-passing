cmake_minimum_required(VERSION 2.8)

project(os-net-descriptor-passing)

include_directories(${PROJECT_SOURCE_DIR})

add_library(common_utils "utils/utils.hpp")
set_target_properties(common_utils PROPERTIES LINKER_LANGUAGE CXX)
add_library(server-lib "server/server.h" "server/server.cpp")
add_library(client-lib "client/client.h" "client/client.cpp")

add_executable(client "mains/client_main.cpp")
add_executable(server "mains/server_main.cpp")

target_link_libraries(client client-lib common_utils -pthread)
target_link_libraries(server server-lib common_utils -pthread)
