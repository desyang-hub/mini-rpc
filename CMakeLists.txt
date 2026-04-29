cmake_minimum_required(VERSION 3.20)

project(minirpc VERSION 1.5.1)

# setting luchange standerd
set(CMAKE_CXX_STANDARD 17) # equal -std=c++14
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set output save path
# set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)


include(FetchContent)
#start=============== dependencies ===============
# 使用 FetchContent 自动下载 nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG          v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(nlohmann_json_interface INTERFACE)
target_include_directories(nlohmann_json_interface INTERFACE 
    ${nlohmann_json_SOURCE_DIR}/include)
#end =============== dependencies ===============


# nacos-cpp
FetchContent_Declare(
    nacos_cpp
    GIT_REPOSITORY https://github.com/nacos-group/nacos-sdk-cpp.git
    GIT_TAG        v1.1.1  # 推荐使用较新稳定版，支持 CMake
)
FetchContent_MakeAvailable(nacos_cpp)



# 编译链接库
#start=============== common ===============
# 创建静态库
add_library(common STATIC
    src/minirpc/common/logger.cc
    src/minirpc/common/timeStamp.cc
    src/minirpc/common/utils.cc
    src/minirpc/core/nacos_config.cc
)
# 创建动态库
# add_library(protocol SHARED 
#     source1.cpp 
#     source2.cpp)

# 为common添加包含目录
target_include_directories(common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  # 安装后头文件会放到 include 目录
)
#end =============== common ===============


#start=============== core ===============
# 创建静态库
add_library(core STATIC
    src/minirpc/core/RpcServer.cc
    src/minirpc/core/RpcClient.cc
    src/minirpc/core/RpcConnection.cc
    src/minirpc/core/RpcConnectionPool.cc
    src/minirpc/core/TcpConnectionPoolFactory.cc
    src/minirpc/core/utils.cc
)
# 创建动态库
# add_library(protocol SHARED 
#     source1.cpp 
#     source2.cpp)

# 为common添加包含目录
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  # 安装后头文件会放到 include 目录
)
target_link_libraries(core PRIVATE 
    net
    protocol
    common
)
#end =============== core ===============

#start=============== net ===============
# 创建静态库
add_library(net STATIC 
    src/minirpc/net/EventLoop.cc
    src/minirpc/net/Poller.cc
    src/minirpc/net/Channel.cc
    src/minirpc/net/DefaultPoller.cc
    src/minirpc/net/EpollPoller.cc
    src/minirpc/net/TcpServer.cc
    src/minirpc/net/utils.cc
)
# 创建动态库
# add_library(protocol SHARED 
#     source1.cpp 
#     source2.cpp)

# 为net添加包含目录
target_include_directories(net PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  # 安装后头文件会放到 include 目录
    # /usr/local/include/nacos # libnacos_cli.so
    ${nacos_cpp_SOURCE_DIR}/include
)

# target_link_directories(net PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/lib
# )


target_link_libraries(net PRIVATE 
    protocol
    common
    nacos-cli-static # 静态链接库
    curl
    z
)
#end =============== net ===============


#start=============== protocol ===============
# 创建静态库
add_library(protocol STATIC 
    src/minirpc/protocol/Serialize.cc
)
# 创建动态库
# add_library(protocol SHARED 
#     source1.cpp 
#     source2.cpp)

# 为protocol添加包含目录
target_include_directories(protocol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  # 安装后头文件会放到 include 目录
)
target_link_libraries(protocol PUBLIC
    common
    nlohmann_json_interface
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
add_subdirectory(example)
#end =============== example ===============



#start=============== test ===============
# 测试相关设置
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    
    # 查找Google Test
    find_package(GTest REQUIRED)
    
    # 或者从源码构建Google Test（推荐方式）
    # include(FetchContent)
    # FetchContent_Declare(
    #     googletest
    #     URL https://github.com/google/googletest/archive/03597a01ee50ed33e9fd7188aa8c3dfb7f2ff3b6.zip
    # )
    # # For Windows: Prevent overriding the parent project's compiler/linker settings
    # set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # FetchContent_MakeAvailable(googletest)


    # 添加测试子目录
    add_subdirectory(tests)
endif()
#end =============== test ===============


# ================== Export Configuration ==================
# 安装头文件和库（已有，无需修改）
install(TARGETS common core net protocol nlohmann_json_interface
    EXPORT minirpc
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

# 安装 nlohmann_json 的头文件（重要！）
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