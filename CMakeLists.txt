cmake_minimum_required(VERSION 3.20)

project(minirpc VERSION 0.1.0)

# setting luchange standerd
set(CMAKE_CXX_STANDARD 17) # equal -std=c++14
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set output save path
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# 编译链接库
#start=============== common ===============
# 创建静态库
add_library(common STATIC 
    src/minirpc/common/logger.cc
    src/minirpc/common/timeStamp.cc
    src/minirpc/common/utils.cc
)
# 创建动态库
# add_library(protocol SHARED 
#     source1.cpp 
#     source2.cpp)

# 为common添加包含目录
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
#end =============== common ===============


#start=============== core ===============
# 创建静态库
add_library(core STATIC 
    src/minirpc/core/RpcServer.cc
)
# 创建动态库
# add_library(protocol SHARED 
#     source1.cpp 
#     source2.cpp)

# 为common添加包含目录
target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
#end =============== core ===============

#start=============== net ===============
# 创建静态库
add_library(net STATIC 
    src/minirpc/net/EventLoop.cc
    src/minirpc/net/Poller.cc
    src/minirpc/net/Channel.cc
)
# 创建动态库
# add_library(protocol SHARED 
#     source1.cpp 
#     source2.cpp)

# 为net添加包含目录
target_include_directories(net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
#end =============== protocol ===============

#start=============== main ===============
# 添加可执行文件
add_executable(minirpc_main
    src/main.cc
)

# 配置链接库
target_link_libraries(minirpc_main 
    protocol
    common
    core
)
#end =============== main ===============


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
