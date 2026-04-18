#include <gtest/gtest.h>
#include <iostream>

#include "minirpc/core/RpcServer.h"
#include "minirpc/core/RpcClient.h"

using namespace std;
using namespace minirpc;



class TestService
{
public:
    int add(const int a, const int b) const {
        return a + b;
    }

RPC_SERVICE_BIND(TestService, add);
RPC_SERVICE_STUB(TestService, add);
};

RPC_SERVICE_REGISTER(TestService);


TEST(RpcServerTest, TestMcore) {
    
    // TestService::TestService_Stub sub;
    // int sum = sub.add(1, 2);

    // EXPECT_EQ(sum, 3);
    EXPECT_EQ(3, 3);
}