# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(miinrpc_tests)

# 创建测试可执行文件
add_executable(test_common 
    test_common.cc
)
add_executable(test_protocol 
    test_protocol.cc
)
add_executable(test_core
    test_core.cc
)

# 链接库和gtest
target_link_libraries(test_common 
    common
    gtest 
    gtest_main
    # GTest::gtest 
    # GTest::gtest_main
    # GTest::gmock
    # GTest::gmock_main
)

target_link_libraries(test_protocol 
    protocol
    common
    gtest 
    gtest_main
    # GTest::gtest 
    # GTest::gtest_main
    # GTest::gmock
    # GTest::gmock_main
)

target_link_libraries(test_core
    protocol
    common
    core
    gtest 
    gtest_main
    # GTest::gtest 
    # GTest::gtest_main
    # GTest::gmock
    # GTest::gmock_main
)

# 添加测试
add_test(NAME test_common_run COMMAND test_common)
add_test(NAME test_protocol_run COMMAND test_protocol)
add_test(NAME test_core_run COMMAND test_core)