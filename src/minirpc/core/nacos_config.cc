#include "minirpc/core/nacos_config.h"
#include <cstdlib>

namespace minirpc {

std::string GetNacosServerAddr() {
    const char* env = std::getenv("NACOS_SERVER_ADDR");
    return env ? std::string(env) : "127.0.0.1:8848";
}

std::string GetNacosServerHost() {
    const char* env = std::getenv("NACOS_SERVER_HOST");
    return env ? std::string(env) : "127.0.0.1";
}

int GetNacosServerPort() {
    const char* env = std::getenv("NACOS_SERVER_PORT");
    return env ? std::atoi(env) : 8848;
}

} // namespace minirpc
