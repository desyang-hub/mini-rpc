/**
 * @FilePath     : /mini-rpc/src/minirpc/core/ServiceInstanceCache.cc
 * @Description  : ServiceInstanceCache 实现
 * @Author       : desyang
 * @Date         : 2026-06-18
 **/

#include "minirpc/core/ServiceInstanceCache.h"
#include "minirpc/common/logger.h"

namespace minirpc
{

ServiceInstanceCache::ServiceInstanceCache()
{
    // 创建 Nacos 工厂
    nacos::Properties configProps;
    configProps[nacos::PropertyKeyConst::SERVER_ADDR] = "127.0.0.1:8848";
    nacos::INacosServiceFactory* factory =
        nacos::NacosFactoryFactory::getNacosFactory(configProps);
    nacos::ResourceGuard<nacos::INacosServiceFactory> _guardFactory(factory);

    namingSvc_ = std::unique_ptr<nacos::NamingService>(factory->CreateNamingService());
}

ServiceInstanceCache::~ServiceInstanceCache()
{
    shutdown();
}

void ServiceInstanceCache::shutdown()
{
    // 防止重复销毁（静态析构时可能被多次调用）
    bool expected = false;
    if (!is_destroyed_.compare_exchange_strong(expected, true)) {
        return;
    }

    // Nacos SDK 内部通过 refCount 管理 EventListener 生命周期：
    // unsubscribe 时 SDK 会 decRef，ref==0 时 SDK 会自动 delete listener。
    // 我们只需要调用 unsubscribe，SDK 会接管销毁。
    // listeners_ 中的 shared_ptr 使用 NoDelete deleter，不会再次 delete。
    for (auto& [name, listener] : listeners_) {
        try {
            namingSvc_->unsubscribe(name, listener.get());
        } catch (const std::exception& e) {
            LOG_ERROR("Unsubscribe failed for %s: %s", name.c_str(), e.what());
        }
    }
    // 清除 map 后 shared_ptr 会销毁，但 NoDelete deleter 不会 delete listener

    // 最后销毁 namingSvc_（内部 EventDispatcher 析构会 purgeAllListeners，
    // 对未被 unsubscribe 的 listener 做最后兜底 delete）
    namingSvc_.reset();
}

void ServiceInstanceCache::subscribeService(const std::string& serviceName)
{
    // 使用独立的 subscribeMutex_ 防止多线程并发重复订阅
    std::lock_guard<std::mutex> lock(subscribeMutex_);

    // 检查是否已订阅
    auto it = listeners_.find(serviceName);
    if (it != listeners_.end()) {
        return;
    }

    // 创建监听器 — 使用 NoDelete deleter，不真正删除 listener（SDK 管理生命周期）
    ServiceChangeListener* raw = new ServiceChangeListener(instanceCache_);
    ListenerPtr listener(raw, NoDelete{});

    // 将裸指针传给 Nacos SDK（SDK 通过 refCount 管理，拥有所有权）
    try {
        namingSvc_->subscribe(serviceName, raw);
        LOG_INFO("Subscribed to service: %s", serviceName.c_str());
    } catch (const nacos::NacosException& e) {
        LOG_ERROR("Subscribe failed for %s: %s", serviceName.c_str(), e.what());
        delete raw; // subscribe 失败，SDK 未接管，我们自己删除
        return;
    }

    // 记录 listener，用于后续 unsubscribe
    listeners_[serviceName] = listener;

    // 订阅后立即尝试一次查询，以获取当前实例（Nacos 订阅后可能不会立即推送）
    refreshInstance(serviceName);
}

std::list<nacos::Instance> ServiceInstanceCache::getInstances(const std::string& serviceName) const
{
    std::shared_lock<std::shared_mutex> lock(cacheMutex_);
    auto it = instanceCache_.find(serviceName);
    if (it != instanceCache_.end()) {
        return it->second;
    }
    return {};
}

void ServiceInstanceCache::refreshInstance(const std::string& serviceName)
{
    try {
        std::list<nacos::Instance> instances = namingSvc_->getAllInstances(serviceName);
        std::unique_lock<std::shared_mutex> lock(cacheMutex_);
        instanceCache_[serviceName] = std::move(instances);
        LOG_INFO("Refreshed instances for %s, count=%zu",
                 serviceName.c_str(), instanceCache_[serviceName].size());
    } catch (const nacos::NacosException& e) {
        LOG_ERROR("Refresh failed for %s: %s", serviceName.c_str(), e.what());
    }
}

} // namespace minirpc
