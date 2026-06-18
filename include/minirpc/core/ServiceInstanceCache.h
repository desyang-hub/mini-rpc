/**
 * @FilePath     : /mini-rpc/include/minirpc/core/ServiceInstanceCache.h
 * @Description  : Nacos 服务实例订阅缓存 - 通过 subscribe 模式获取服务实例变更通知，
 *                 替代每次 RPC 请求时同步调用 getAllInstances 的做法。
 * @Author       : desyang
 * @Date         : 2026-06-18
 **/
#pragma once

#include <unordered_map>
#include <string>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <atomic>
#include <Nacos.h>

namespace minirpc
{

/// @brief Nacos 服务变更监听器
/// 当 Nacos 中某个服务的实例列表发生变更时，SDK 后台线程会回调此方法，
/// 将最新的实例列表写入缓存。
///
/// ⚠️ 生命周期说明（非常重要）：
/// Nacos SDK 通过 refCount 引用计数完全管理 EventListener 的生命周期：
///   - addListener 时 SDK 调用 incRef()
///   - removeListener 时 SDK 调用 decRef()，ref==0 时 SDK 自动 delete
///   - EventDispatcher 析构时 purgeAllListeners() 也会 delete 所有 listener
///   - ~EventListener() 中有一个断言：NACOS_ASSERT(refCnt() == 0)
///
/// 因此我们绝不能自己 delete listener，也不能让 shared_ptr 的 deleter 调用 delete。
class ServiceChangeListener : public nacos::EventListener {
private:
    // 服务名 -> 实例列表 的缓存（通过引用持有外部缓存）
    std::unordered_map<std::string, std::list<nacos::Instance>>& instanceCache_;
    mutable std::shared_mutex cacheMutex_;

public:
    explicit ServiceChangeListener(
        std::unordered_map<std::string, std::list<nacos::Instance>>& cache)
        : instanceCache_(cache)
    {}

    /// @brief Nacos SDK 回调：服务实例列表变更时触发
    /// 注意：Nacos SDK 的 receiveNamingInfo 签名是 const ServiceInfo&，但 getName/getHosts 非 const，
    /// 所以需要 const_cast。
    void receiveNamingInfo(const nacos::ServiceInfo& serviceInfo) override {
        auto& info = const_cast<nacos::ServiceInfo&>(serviceInfo);
        std::string name = std::string(info.getName());
        std::list<nacos::Instance> hosts = info.getHosts();

        std::unique_lock<std::shared_mutex> lock(cacheMutex_);
        instanceCache_[name] = std::move(hosts);
    }
};

/// @brief 空 deleter — shared_ptr 不删除 listener，生命周期由 Nacos SDK 管理
struct NoDelete {
    template<typename T>
    void operator()(T*) const noexcept {}
};

/// @brief 服务实例缓存管理器
/// 负责创建 nacos::NamingService，对指定服务名发起 subscribe，
/// 并通过 EventListener 回调实时更新缓存。
///
/// 生命周期说明：
/// - Nacos SDK 通过 refCount 管理 EventListener，unsubscribe 时 SDK 会 delete listener。
/// - 我们用 shared_ptr + NoDelete deleter 持有 listener，仅为了在 map 中安全存储。
/// - 析构时：先 unsubscribe（SDK 负责 delete listener），再销毁 namingSvc_。
using ListenerPtr = std::shared_ptr<ServiceChangeListener>;

class ServiceInstanceCache {
private:
    std::unordered_map<std::string, std::list<nacos::Instance>> instanceCache_;
    mutable std::shared_mutex cacheMutex_;

    std::unique_ptr<nacos::NamingService> namingSvc_;
    // serviceName -> listener 映射（shared_ptr + NoDelete，SDK 管理生命周期）
    std::unordered_map<std::string, ListenerPtr> listeners_;

    // 标记是否已销毁，避免析构顺序问题导致的双重析构
    std::atomic<bool> is_destroyed_{false};

    // 保护 subscribeService：多线程并发首次调用时只 subscribe 一次
    mutable std::mutex subscribeMutex_;

public:
    ServiceInstanceCache();
    ~ServiceInstanceCache();

    /// @brief 订阅指定服务名，后续该服务的实例变更会回调到 listener
    /// @param serviceName 服务名（与 RpcClient::Call 中的 serviceName 对应）
    void subscribeService(const std::string& serviceName);

    /// @brief 获取指定服务的所有实例（从缓存读取，线程安全）
    /// @return 实例列表；如果尚未订阅或没有实例，返回空列表
    std::list<nacos::Instance> getInstances(const std::string& serviceName) const;

    /// @brief 初始化阶段手动查询一次实例列表并写入缓存（兜底）
    void refreshInstance(const std::string& serviceName);

    /// @brief 主动销毁 NamingService，避免静态析构顺序问题
    void shutdown();
};

} // namespace minirpc
