add_executable(client_p 
    Client.cc
)

target_link_libraries(client_p PRIVATE
    protocol
    core
    net
    common
    nlohmann_json::nlohmann_json
    muduo_net
    muduo_base
)

# target_compile_options(client_p PRIVATE -fsanitize=address -fno-omit-frame-pointer -O1)
# target_link_options(client_p PRIVATE -fsanitize=address)


add_executable(server_p
    UserService.cc
    # Server.cc
    server.cc
)

target_link_libraries(server_p PRIVATE
    nacos-cli
    protocol
    core
    net
    common
    nlohmann_json::nlohmann_json
    muduo_net
    muduo_base
)


# target_compile_options(server_p PRIVATE -fsanitize=address -fno-omit-frame-pointer -O1)
# target_link_options(server_p PRIVATE -fsanitize=address)