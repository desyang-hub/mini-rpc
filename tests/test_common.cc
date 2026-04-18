#include <gtest/gtest.h>
#include <iostream>
#include "minirpc/common/utils.h"  // 假设这是您要测试的头文件
#include "minirpc/common/logger.h"

using namespace std;
using namespace minirpc;

// 假设utils.h中有一些函数或类可以测试
class UtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 在每个测试之前运行
        std::cout << "SetUp" << std::endl;
    }

    void TearDown() override {
        // 在每个测试之后运行
        std::cout << "TearDown" << std::endl;
    }
};

// 测试示例 - 您需要根据实际的utils.h内容修改
TEST_F(UtilsTest, TestFunctionName) {
    // 实际测试代码 - 根据您的utils.h内容调整
    // int result = your_function();
    // EXPECT_EQ(result, expected_value);
    std::cout << "Testing" << std::endl;
    EXPECT_TRUE(true);  // 占位符 - 替换为实际测试
}

// 参数化测试示例
class ParameterizedTest : public ::testing::TestWithParam<std::pair<int, int>> {
protected:
    int input;
    int expected;

    void SetUp() override {
        input = GetParam().first;
        expected = GetParam().second;
    }
};

INSTANTIATE_TEST_SUITE_P(
    TestValues,
    ParameterizedTest,
    ::testing::Values(
        std::make_pair(1, 1),
        std::make_pair(2, 4),
        std::make_pair(3, 9)
    )
);

TEST_P(ParameterizedTest, TestSquare) {
    // 根据您的实际函数调整
    // int result = square_function(input);
    // EXPECT_EQ(result, expected);
}


TEST(LoggerKitTest, TestLogger) {
    ENABLE_ASYNC_LOGING();
    LOG_INFO("info log");
    LOG_DEBUG("debug log");
    LOG_ERROR("error log");
}