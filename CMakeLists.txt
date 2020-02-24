cmake_minimum_required(VERSION 3.10)
set(CXX_FLAGS -g)
ENABLE_LANGUAGE(ASM)

add_library(
    coroutine-net
    coroutine.cpp
    co_context.c
    co_context.S
    hook.cpp
    event_loop.cpp
    timer_wheel.cpp
)

add_executable(
    example_coroutine
    example_coroutine.cpp
)

add_executable(
    example_echo_client
    example_echo_client.cpp
)

add_executable(
    example_echo_server
    example_echo_server.cpp
)

add_executable(
    test_timer_wheel
    test_timer_wheel.cpp
)

target_link_libraries(example_coroutine coroutine-net dl)
target_link_libraries(example_echo_client coroutine-net dl)
target_link_libraries(example_echo_server coroutine-net dl)
target_link_libraries(test_timer_wheel coroutine-net dl)
