#pragma once

#include <iostream>
#include <string>


/**
 * @brief 这里的实现只是暂时的，由于nacos服务实例之间冲突，目前只能够暂时通过curl来进行服务获取，而服务缓存逻辑需要自己来实现
 */

namespace minirpc
{
// 回调函数：将 curl 获取的数据追加到 std::string
// Note: WriteCallback is file-local (defined in utils.cc), not exposed in header.

// 简易解析（不依赖 JSON 库）
std::string parseFirstInstance(const std::string &jsonStr);


std::string getServiceAddress(const std::string& srvName);

    
} // namespace minirpc
