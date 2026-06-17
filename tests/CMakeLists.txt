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

add_executable(test_net
test_net.cc
)

add_executable(test_security
    test_security.cc
)

add_executable(test_server
    test_server.cc
)

target_link_libraries(test_server
    protocol
    core
    net
    common
)

# target_compile_options(test_server PRIVATE -fsanitize=address -fno-omit-frame-pointer -O1)
# target_link_options(test_server PRIVATE -fsanitize=address)

# 链接库和gtest
target_link_libraries(test_common 
    common
    gtest 
    gtest_main
)

target_link_libraries(test_protocol 
    protocol
    common
    gtest 
    gtest_main
)

target_link_libraries(test_core
    protocol
    core
    common
    gtest 
    gtest_main
)


target_link_libraries(test_net
    protocol
    core
    net
    common
    gtest 
    gtest_main
)

target_link_libraries(test_security
    protocol
    common
    gtest 
    gtest_main
)

# 添加测试
add_test(NAME test_common_run COMMAND test_common)
add_test(NAME test_protocol_run COMMAND test_protocol)
# add_test(NAME test_core_run COMMAND test_core)
add_test(NAME test_net_run COMMAND test_net)
add_test(NAME test_security_run COMMAND test_security)
