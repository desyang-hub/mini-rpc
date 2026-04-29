#pragma once

#include <string>

namespace minirpc {

// Returns the Nacos server address for service discovery.
// Reads from env var NACOS_SERVER_ADDR, falls back to "127.0.0.1:8848".
std::string GetNacosServerAddr();

// Returns the Nacos server host (without port) for service registration.
// Reads from env var NACOS_SERVER_HOST, falls back to "127.0.0.1".
std::string GetNacosServerHost();

// Returns the Nacos server port for service registration.
// Reads from env var NACOS_SERVER_PORT, falls back to 8848.
int GetNacosServerPort();

} // namespace minirpc
