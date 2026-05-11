project(config C CXX)

add_executable(load_conf load_conf.cc)

target_include_directories(load_conf
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)

# 将 config.toml 从源码目录复制到构建目录（保持同名）
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.toml   # 源文件
    ${CMAKE_CURRENT_BINARY_DIR}/config.toml   # 目标文件
    COPYONLY                         # 关键！表示仅复制，不处理变量替换
)