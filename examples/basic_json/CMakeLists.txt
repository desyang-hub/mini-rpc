add_executable(json_client Client.cc)
target_link_libraries(json_client PRIVATE
    core net protocol common nlohmann_json::nlohmann_json
)

add_executable(json_server UserService.cc Server.cc)
target_link_libraries(json_server PRIVATE
    core net protocol common nacos::nacos-cli nlohmann_json::nlohmann_json muduo_net muduo_base
)

configure_file(config.toml ${CMAKE_CURRENT_BINARY_DIR}/config.toml COPYONLY)
