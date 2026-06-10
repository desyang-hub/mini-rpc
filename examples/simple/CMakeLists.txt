add_executable(client 
    Client.cc
)

target_link_libraries(client PRIVATE
    protocol
    core
    net
    common
    nlohmann_json::nlohmann_json
)


add_executable(server 
    UserService.cc
    Server.cc
)

target_link_libraries(server PRIVATE
    protocol
    core
    net
    common
    nlohmann_json::nlohmann_json
)