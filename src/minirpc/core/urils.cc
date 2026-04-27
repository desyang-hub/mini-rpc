#include "minirpc/core/utils.h"

#include "minirpc/common/logger.h"

#include <curl/curl.h>  // libcurl 主头文件
#include <stdexcept>

#include "nlohmann/json.hpp"


namespace minirpc
{


size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

using json = nlohmann::json;

// 简易解析（不依赖 JSON 库）
std::string parseFirstInstance(const std::string &jsonStr) {


    LOG_INFO("jsonStr: %s", jsonStr.c_str());

    json a = json::parse(jsonStr);
    a = a["hosts"];
    
    if (a.size()) {
        return a[0]["ip"].get<std::string>() + ":" + std::to_string(a[0]["port"].get<int>());
    }

    throw std::runtime_error("none any service instance");
}


// TODO: 暂时使用8848这个临时端口，后期需要用域名来替换
std::string getServiceAddress(const std::string& srvName) {
    CURL *curl = curl_easy_init();
    std::string response;
    if (curl) {
        std::string url = "http://127.0.0.1:8848/nacos/v1/ns/instance/list?serviceName=" + srvName;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    // 解析 response，提取 host:port
    return parseFirstInstance(response); // 例如 "127.0.0.1:2000"
}
    
    
} // namespace minirpc
