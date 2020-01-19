cmake_minimum_required(VERSION 3.10)
set(CXX_FLAGS -g)
ENABLE_LANGUAGE(ASM)

add_library(
    coroutine
    coroutine.cpp
    co_context.c
    co_context.S
)

add_executable(
    example_coroutine
    example_coroutine.cpp
)

add_executable(
    example_echo_client
    coroutine_net.cpp
    event_loop.cpp
    example_echo_client.cpp
)

add_executable(
    example_echo_server
    coroutine_net.cpp
    event_loop.cpp
    example_echo_server.cpp
)

target_link_libraries(example_coroutine coroutine)
target_link_libraries(example_echo_client coroutine)
target_link_libraries(example_echo_server coroutine)