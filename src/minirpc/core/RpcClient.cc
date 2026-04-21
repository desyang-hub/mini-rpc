#include "minirpc/core/RpcClient.h"


namespace minirpc
{

int RpcClient::fd_ = -1;
uint64_t RpcClient::request_id_ = 0;
std::unordered_map<uint64_t, std::promise<Response>> RpcClient::promiseMap_;

std::mutex RpcClient::send_lock_;
    
} // namespace minirpc
