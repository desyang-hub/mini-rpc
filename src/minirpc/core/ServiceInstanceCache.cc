#include "minirpc/core/ServiceInstanceCache.h"
#include "minirpc/common/logger.h"

namespace minirpc
{

ServiceInstanceCache::ServiceInstanceCache(const std::string& nacosAddr)
    : nacos_addr_(nacosAddr)
{
    nacos::Properties props;
    props[nacos::PropertyKeyConst::SERVER_ADDR] =
        nacos_addr_.empty() ? "127.0.0.1:8848" : nacos_addr_;

    nacos::INacosServiceFactory* factory =
        nacos::NacosFactoryFactory::getNacosFactory(props);
    nacos::ResourceGuard<nacos::INacosServiceFactory> guard(factory);

    namingSvc_ = std::unique_ptr<nacos::NamingService>(factory->CreateNamingService());
}

void ServiceInstanceCache::shutdown()
{
    bool expected = false;
    if (!is_destroyed_.compare_exchange_strong(expected, true)) return;

    for (auto& [name, listener] : listeners_) {
        try {
            namingSvc_->unsubscribe(name, listener.get());
        } catch (const std::exception& e) {
            LOG_ERROR("Unsubscribe %s failed: %s", name.c_str(), e.what());
        }
    }
    namingSvc_.reset();
}

void ServiceInstanceCache::subscribeService(const std::string& name)
{
    std::lock_guard<std::mutex> lock(subscribeMutex_);

    if (listeners_.count(name)) return;

    ServiceChangeListener* raw = new ServiceChangeListener(instanceCache_);
    ListenerPtr listener(raw, NoDelete{});

    try {
        namingSvc_->subscribe(name, raw);
        LOG_INFO("Subscribed to service: %s", name.c_str());
    } catch (const nacos::NacosException& e) {
        LOG_ERROR("Subscribe %s failed: %s", name.c_str(), e.what());
        delete raw;
        return;
    }

    listeners_[name] = listener;
    refreshInstance(name);
}

std::list<nacos::Instance> ServiceInstanceCache::getInstances(const std::string& name) const
{
    std::shared_lock<std::shared_mutex> lock(cacheMutex_);
    auto it = instanceCache_.find(name);
    if (it != instanceCache_.end()) return it->second;
    return {};
}

void ServiceInstanceCache::refreshInstance(const std::string& name)
{
    try {
        auto instances = namingSvc_->getAllInstances(name);
        std::unique_lock<std::shared_mutex> lock(cacheMutex_);
        instanceCache_[name] = std::move(instances);
    } catch (const nacos::NacosException& e) {
        LOG_ERROR("Refresh %s failed: %s", name.c_str(), e.what());
    }
}

} // namespace minirpc
