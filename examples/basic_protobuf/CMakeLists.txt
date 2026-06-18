find_package(Protobuf REQUIRED)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/protoc/user.proto)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${PROTO_FILE})

add_executable(pb_client Client.cc ${PROTO_SRC})
target_include_directories(pb_client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(pb_client PRIVATE
    ${Protobuf_LIBRARIES}
    core net protocol common nlohmann_json::nlohmann_json
)

add_executable(pb_server UserService.cc Server.cc ${PROTO_SRC})
target_include_directories(pb_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(pb_server PRIVATE
    ${Protobuf_LIBRARIES}
    core net protocol common nacos::nacos-cli nlohmann_json::nlohmann_json muduo_net muduo_base
)

configure_file(config.toml ${CMAKE_CURRENT_BINARY_DIR}/config.toml COPYONLY)
