#include "minirpc/core/utils.h"
#include "minirpc/core/nacos_config.h"

#include "minirpc/common/logger.h"
#include "minirpc/common/Random.h"

#include <curl/curl.h>  // libcurl 主头文件
#include <stdexcept>
#include <assert.h>

#include "nlohmann/json.hpp"


namespace minirpc
{

namespace {

size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

using json = nlohmann::json;

} // namespace

// 简易解析（不依赖 JSON 库）
std::string parseFirstInstance(const std::string &jsonStr) {

    if (jsonStr.empty()) {
        throw std::runtime_error("empty response from nacos service registry");
    }

    json j;
    try {
        j = json::parse(jsonStr);
    } catch (const json::parse_error&) {
        throw std::runtime_error("failed to parse nacos response as JSON: " + jsonStr);
    }

    auto hosts = j.find("hosts");
    if (hosts == j.end() || !hosts->is_array() || hosts->empty()) {
        throw std::runtime_error("no service instances found (nacos may not have this service registered)");
    }

    const auto &a = *hosts;
    int idx = Random::RandInt(0, static_cast<int>(a.size()));
    const auto &instance = a[idx];

    if (!instance.contains("ip") || !instance.contains("port")) {
        throw std::runtime_error("invalid service instance: missing ip or port field");
    }

    return instance["ip"].get<std::string>() + ":" + std::to_string(instance["port"].get<int>());
}


std::string getServiceAddress(const std::string& srvName) {
    CURL *curl = curl_easy_init();
    std::string response;
    if (curl) {
        std::string url = "http://" + GetNacosServerAddr() + "/nacos/v1/ns/instance/list?serviceName=" + srvName;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    // 解析 response，提取 host:port
    return parseFirstInstance(response); // 例如 "127.0.0.1:2000"
}
    
    
} // namespace minirpc
