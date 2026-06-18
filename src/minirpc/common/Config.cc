/**
 * @FilePath     : /mini-rpc/src/minirpc/common/Config.cc
 * @Description  : TOML configuration loader implementation
 * @Author       : desyang
 * @Date         : 2026-06-18
 **/

#include "minirpc/common/Config.h"
#include "toml++/toml.hpp"

#include <iostream>

namespace minirpc
{

Config loadConfig(const std::string& filepath)
{
    Config cfg;

    try {
        auto toml = toml::parse_file(filepath);
        if (toml.empty()) {
            return cfg;
        }

        // [server]
        cfg.port = toml["server"]["port"].value_or(cfg.port);
        cfg.listen_host = toml["server"]["listen_host"].value_or(cfg.listen_host.c_str());

        // [registry]
        cfg.registry_address = toml["registry"]["address"].value_or(cfg.registry_address.c_str());
        cfg.registry_group = toml["registry"]["group"].value_or(cfg.registry_group.c_str());
        cfg.registry_cluster = toml["registry"]["cluster"].value_or(cfg.registry_cluster.c_str());
    } catch (const std::exception& e) {
        // File not found or parse error — use defaults silently
    }

    return cfg;
}

} // namespace minirpc
