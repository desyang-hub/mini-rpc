#pragma once

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <Nacos.h>

namespace minirpc
{

// Nacos service change listener - receives callbacks when instances change
class ServiceChangeListener : public nacos::EventListener
{
private:
    std::unordered_map<std::string, std::list<nacos::Instance>>& cache_;
    mutable std::shared_mutex cacheMutex_;

public:
    explicit ServiceChangeListener(
        std::unordered_map<std::string, std::list<nacos::Instance>>& c)
        : cache_(c) {}

    void receiveNamingInfo(const nacos::ServiceInfo& info) override
    {
        auto& mutableInfo = const_cast<nacos::ServiceInfo&>(info);
        std::string name(mutableInfo.getName());
        std::list<nacos::Instance> hosts = mutableInfo.getHosts();

        std::unique_lock<std::shared_mutex> lock(cacheMutex_);
        cache_[name] = std::move(hosts);
    }
};

// No-op deleter - Nacos SDK manages EventListener lifecycle via refCount
struct NoDelete {
    template<typename T> void operator()(T*) const noexcept {}
};

using ListenerPtr = std::shared_ptr<ServiceChangeListener>;

// Service instance cache - subscribes to Nacos for real-time instance updates
class ServiceInstanceCache
{
private:
    std::unordered_map<std::string, std::list<nacos::Instance>> instanceCache_;
    mutable std::shared_mutex cacheMutex_;

    std::unique_ptr<nacos::NamingService> namingSvc_;
    std::string nacos_addr_;
    std::unordered_map<std::string, ListenerPtr> listeners_;
    std::atomic<bool> is_destroyed_{false};
    mutable std::mutex subscribeMutex_;

    void refreshInstance(const std::string& serviceName);

public:
    explicit ServiceInstanceCache(const std::string& nacosAddr = "127.0.0.1");
    ~ServiceInstanceCache() { shutdown(); }

    // Subscribe to a service (thread-safe, idempotent)
    void subscribeService(const std::string& serviceName);

    // Get cached instances (thread-safe)
    std::list<nacos::Instance> getInstances(const std::string& serviceName) const;

    // Manually shut down (unsubscribe all)
    void shutdown();
};

} // namespace minirpc
