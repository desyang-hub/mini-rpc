/**
 * @FilePath     : /mini-rpc/include/minirpc/common/Config.h
 * @Description  : Configuration file loader (TOML format)
 * @Author       : desyang
 * @Date         : 2026-06-18
 **/
#pragma once

#include <string>

namespace minirpc
{

/// @brief Default configuration values when config file is absent or keys are missing
struct DefaultConfig
{
    static constexpr int kDefaultPort = 8080;
    static constexpr const char* kDefaultListenHost = "0.0.0.0";
    static constexpr const char* kDefaultRegistryAddress = "127.0.0.1";
    static constexpr const char* kDefaultGroupName = "DefaultGroup";
    static constexpr const char* kDefaultClusterName = "DefaultCluster";
};

/// @brief Parsed configuration from a TOML file.
///        Falls back to defaults for missing keys/files.
struct Config
{
    // Server
    int port = DefaultConfig::kDefaultPort;
    std::string listen_host = DefaultConfig::kDefaultListenHost;

    // Service registry (Nacos)
    std::string registry_address = DefaultConfig::kDefaultRegistryAddress;
    std::string registry_group = DefaultConfig::kDefaultGroupName;
    std::string registry_cluster = DefaultConfig::kDefaultClusterName;
};

/// @brief Load configuration from a TOML file.
/// @param filepath  Path to the .toml file (default: "config.toml")
/// @return Config struct. Missing keys use default values.
Config loadConfig(const std::string& filepath = "config.toml");

} // namespace minirpc
