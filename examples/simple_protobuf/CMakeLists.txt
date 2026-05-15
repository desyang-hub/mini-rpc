cmake_minimum_required(VERSION 3.10)
project(protobuf_demo C CXX)

# === 1. 自动查找 output 目录（包含 include/ 和 lib/）===
execute_process(
    COMMAND bash -c "find ${PROJECT_SOURCE_DIR}/../.. -type d -regex '.*output/include\$' | head -n1 | xargs -r dirname"
    OUTPUT_VARIABLE OUTPUT_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT OUTPUT_PATH)
    message(WARNING "Could not find 'output/include' directory. Trying system Protobuf.")
else()
    set(CMAKE_PREFIX_PATH ${OUTPUT_PATH})
endif()

# === 2. 查找 Protobuf（关键！必须调用）===
find_package(Protobuf REQUIRED)

# === 3. 指定 .proto 文件路径 ===
set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/protoc/user.proto)
if(NOT EXISTS ${PROTO_FILE})
    message(FATAL_ERROR "Proto file not found: ${PROTO_FILE}")
endif()

# === 4. 生成 C++ 代码（.pb.cc 和 .pb.h）===
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${PROTO_FILE})

# === 5. 创建可执行文件 ===
add_executable(client_protobuf
    Client.cc
    ${PROTO_SRC}
)

add_executable(server_protobuf
    UserService.cc
    Server.cc
    ${PROTO_SRC}
)

# === 6. 设置头文件搜索路径（生成的 .pb.h 在构建目录）===
target_include_directories(client_protobuf 
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(server_protobuf 
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)
# target_include_directories(simple_protobuf PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# === 7. 链接 Protobuf 库（否则会链接错误）===
target_link_libraries(client_protobuf 
    ${Protobuf_LIBRARIES}
    net
    protocol
    protobuf
    core
)

target_link_libraries(server_protobuf 
    ${Protobuf_LIBRARIES}
    net
    protocol
    protobuf
    core
)
