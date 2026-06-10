# cmake/minirpcVersionConfig.cmake

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/minirpc/version.h"
    @ONLY
)

include_directories(
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ✅ 为所有需要版本信息的目标添加 include 路径
foreach(_target common protocol net core)
    if(TARGET ${_target})
        target_include_directories(${_target} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )
    endif()
endforeach()