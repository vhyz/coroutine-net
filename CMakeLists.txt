set(CXX_FLAGS -g)

add_executable(
    example_coroutine
    example_coroutine.cpp
    coroutine.cpp
)

add_executable(
    example_echo_client
    coroutine.cpp
    coroutine_net.cpp
    event_loop.cpp
    example_echo_client.cpp
)

add_executable(
    example_echo_server
    coroutine.cpp
    coroutine_net.cpp
    event_loop.cpp
    example_echo_server.cpp
)