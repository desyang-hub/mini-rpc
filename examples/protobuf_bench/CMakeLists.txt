find_package(Protobuf REQUIRED)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/protoc/user.proto)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${PROTO_FILE})

# --- Client ---
add_executable(pb_bench_client Client.cc ${PROTO_SRC})
target_include_directories(pb_bench_client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(pb_bench_client PRIVATE
    ${Protobuf_LIBRARIES}
    core net protocol common nlohmann_json::nlohmann_json muduo_net muduo_base nacos::nacos-cli
)

# --- Server ---
add_executable(pb_bench_server UserService.cc Server.cc ${PROTO_SRC})
target_include_directories(pb_bench_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(pb_bench_server PRIVATE
    ${Protobuf_LIBRARIES}
    core net protocol common nacos::nacos-cli nlohmann_json::nlohmann_json muduo_net muduo_base
)

# --- Benchmark ---
add_executable(rpcbench rpcbench.cc ${PROTO_SRC})
target_include_directories(rpcbench PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(rpcbench PRIVATE
    ${Protobuf_LIBRARIES}
    core net protocol common nlohmann_json::nlohmann_json muduo_net muduo_base nacos::nacos-cli
)
# target_compile_options(rpcbench PRIVATE -fsanitize=address -fno-omit-frame-pointer -O1)
# target_link_options(rpcbench PRIVATE -fsanitize=address)

configure_file(config.toml ${CMAKE_CURRENT_BINARY_DIR}/config.toml COPYONLY)
