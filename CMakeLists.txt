cmake_minimum_required(VERSION 3.20)

project(minirpc VERSION 3.2.0)
message(STATUS "Build minirpc: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES OFF)
message(STATUS "Build examples: ${BUILD_EXAMPLES}")

# ============================================================
# Dependencies (FetchContent)
# ============================================================
include(FetchContent)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(nacos_cpp
    GIT_REPOSITORY https://github.com/desyang-hub/nacos-sdk-cpp.git
    GIT_TAG v1.1.3
)
FetchContent_MakeAvailable(nacos_cpp)

FetchContent_Declare(muduo
    GIT_REPOSITORY https://github.com/chenshuo/muduo.git
    GIT_TAG v2.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(muduo)

# ============================================================
# Libraries
# ============================================================

# --- common ---
aux_source_directory(src/minirpc/common/ COMMON_SOURCE)
add_library(common ${COMMON_SOURCE})
target_include_directories(common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    SYSTEM PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include  # for toml++
)

# --- protocol (header-only) ---
add_library(protocol INTERFACE)
target_include_directories(protocol INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(protocol INTERFACE
    common nlohmann_json::nlohmann_json
)

# --- net ---
aux_source_directory(src/minirpc/net/ NET_SOURCE)
add_library(net ${NET_SOURCE})
target_include_directories(net
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${muduo_SOURCE_DIR}
)
target_link_libraries(net
    PUBLIC protocol common
    PRIVATE muduo_net muduo_base
)

# --- core ---
aux_source_directory(src/minirpc/core/ CORE_SOURCE)
add_library(core ${CORE_SOURCE})
target_include_directories(core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${nacos_cpp_SOURCE_DIR}/include
        ${muduo_SOURCE_DIR}
)
target_link_libraries(core
    PRIVATE net protocol common nacos::nacos-cli muduo_net muduo_base
)

# ============================================================
# Main executable
# ============================================================
add_executable(minirpc_main src/main.cc)
target_link_libraries(minirpc_main PRIVATE core net protocol common)

# ============================================================
# Examples
# ============================================================
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================
# Tests
# ============================================================
if(NOT DEFINED BUILD_TESTS)
    set(BUILD_TESTS OFF CACHE BOOL "Build tests")
endif()
message(STATUS "Build tests: ${BUILD_TESTS}")

if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()

# ============================================================
# Install
# ============================================================
install(TARGETS common core net protocol nlohmann_json nacos-cli
    EXPORT minirpc
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY ${nlohmann_json_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp")

# ============================================================
# CPack
# ============================================================
include(CPack)
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "minirpc")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
