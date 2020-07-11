add_executable(
    client
    client.cpp
)

add_executable(
    server
    server.cpp
)

target_link_libraries(client coroutine-net)
target_link_libraries(server coroutine-net)