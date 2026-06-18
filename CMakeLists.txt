cmake_minimum_required(VERSION 3.20)

# get minirpc version by minirpc/version.h
# include(cmake/version.cmake)
# minirpc_extract_version()

project(minirpc VERSION 2.0.0)
message(STATUS "Build minirpc: ${MINIRPC_VERSION}")

# setting luchange standerd
set(CMAKE_CXX_STANDARD 17) # equal -std=c++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES OFF)
message(STATUS "Build examples: ${BUILD_EXAMPLES}")

set(LIB_TYPE "")

include(FetchContent)
#start=============== dependencies ===============
# 使用 FetchContent 自动下载 nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG          v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)
# add_library(nlohmann_json_interface ALIAS nlohmann_json::nlohmann_json)

# add_library(nlohmann_json_interface INTERFACE)
# target_include_directories(nlohmann_json_interface INTERFACE
#     ${nlohmann_json_SOURCE_DIR}/include)
#end =============== dependencies ===============


# nacos-cpp
FetchContent_Declare(
    nacos_cpp
    GIT_REPOSITORY https://github.com/desyang-hub/nacos-sdk-cpp.git
    GIT_TAG        v1.1.3  # 推荐使用较新稳定版，支持 CMake
    # 修复 upstream: CMAKE_SOURCE_DIR → CMAKE_CURRENT_SOURCE_DIR
    # 当 FetchContent 把 nacos_cpp 作为子目录引入时，CMAKE_SOURCE_DIR 指向顶层项目目录
    # 导致 install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/nacos ...)
    # 错误地把主项目的头文件安装到 include/nacos/ 下 (含 minirpc/子目录)
    # PATCH_COMMAND    /usr/bin/sed -i "s/CMAKE_SOURCE_DIR/CMAKE_CURRENT_SOURCE_DIR/g" ${CMAKE_CURRENT_BINARY_DIR}/_deps/nacos_cpp-src/CMakeLists.txt
)
FetchContent_MakeAvailable(nacos_cpp)


FetchContent_Declare(
    muduo
    GIT_REPOSITORY https://github.com/chenshuo/muduo.git
    GIT_TAG        v2.0.2  # 建议使用稳定 tag，避免 master 变动
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(muduo)


# 编译链接库
#start=============== common ===============
aux_source_directory(src/minirpc/common/ COMMON_SOURCE)
add_library(common ${LIB_TYPE}
    ${COMMON_SOURCE}
)

# toml++ header-only — lives in include/toml++
target_include_directories(common SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 为 common 添加包含目录
target_include_directories(common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  # 安装后头文件会放到 include 目录
)
#end =============== common ===============


#start=============== core ===============
aux_source_directory(src/minirpc/core/ CORE_SOURCE)
add_library(core ${LIB_TYPE}
    ${CORE_SOURCE}
)

# 为 core 添加包含目录
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  # 安装后头文件会放到 include 目录
    ${nacos_cpp_SOURCE_DIR}/include
    ${muduo_SOURCE_DIR}
)

target_link_libraries(core PRIVATE
    net
    protocol
    common
    nacos::nacos-cli # 静态链接库
    muduo_net
    muduo_base
)
#end =============== core ===============


#start=============== net ===============
# 创建静态库
aux_source_directory(src/minirpc/net/ NET_SOURCE)
add_library(net ${LIB_TYPE}
    ${NET_SOURCE}
)

# 为 net 添加包含目录
target_include_directories(net PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  # 安装后头文件会放到 include 目录
    ${muduo_SOURCE_DIR}
)

target_link_libraries(net PRIVATE
    protocol
    common
    muduo_net
    muduo_base
)
#end =============== net ===============


#start=============== protocol ===============
# 创建静态库
add_library(protocol ${LIB_TYPE}
    src/minirpc/protocol/Serialize.cc
)

# 为 protocol 添加包含目录
target_include_directories(protocol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  # 安装后头文件会放到 include 目录
)

target_link_libraries(protocol PUBLIC
    common
    nlohmann_json::nlohmann_json
)

#end =============== protocol ===============

#start=============== main ===============
# 添加可执行文件
add_executable(minirpc_main
    src/main.cc
)

# 配置链接库
target_link_libraries(minirpc_main PRIVATE
    protocol
    core
    net
    common
)
#end =============== main ===============



#start =============== example ===============
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
#end =============== example ===============


include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/minirpcVersionConfig.cmake)

#start=============== test ===============
# 测试相关设置
if(NOT DEFINED BUILD_TESTS)
    set(BUILD_TESTS OFF CACHE BOOL "Build tests")
endif()
message(STATUS "Build tests: ${BUILD_TESTS}")

if(BUILD_TESTS)
    enable_testing()

    # 通过 FetchContent 获取 GTest(跨平台，CI 无需手动安装)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()
#end =============== test ===============


# ================== Export Configuration ==================
# 安装头文件和库 (已有，无需修改)
install(TARGETS common core net protocol nlohmann_json nacos-cli
    EXPORT minirpc
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

# 安装 nlohmann_json 的头文件 (重要!)
install(DIRECTORY ${nlohmann_json_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp")

# ================== CPack Configuration ==================
# 配置打包工具 (生成 .deb, .rpm, .zip 等)
include(CPack)
set(CPACK_GENERATOR "ZIP") # 开发阶段先用 zip
set(CPACK_PACKAGE_NAME "minirpc")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF) # 避免生成多余的文件夹
