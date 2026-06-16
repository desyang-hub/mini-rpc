/**
 * @FilePath     : /mini-rpc/include/minirpc/core/PendingRequest.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-06-16 10:55:42
 * @LastEditors  : desyang
 * @LastEditTime : 2026-06-16 11:00:34
**/
#pragma once

#include "minirpc/net/TcpClient.h"
#include "minirpc/common/Response.h"

#include <future>

namespace minirpc
{

struct PendingRequest
{
    TcpClientPtr conn;
    std::promise<Response> promise;
};


    
} // namespace minirpc
