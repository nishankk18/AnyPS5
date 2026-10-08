#include <atomic>
#include <condition_variable>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>

#include "prx/libc/include/General.hpp"

namespace {

constexpr int ThrdSuccess = 0;
constexpr int ThrdNomem = 1;
constexpr int MtxPlain = 0x01;
constexpr int MtxTry = 0x02;
constexpr int MtxTimed = 0x04;
constexpr int MtxRecursive = 0x100;

struct GuestMutex {
    std::mutex native;
    std::atomic<std::thread::id> owner;
    unsigned count = 0;
    int type = 0;
};

struct GuestCondition {
    std::condition_variable native;
};

GuestMutex& MutexOf(GuestMutex* const* mutex, const char* function) {
    if (mutex == nullptr || *mutex == nullptr) throw std::invalid_argument(std::string(function) + ": null mutex");
    return **mutex;
}

GuestCondition& ConditionOf(GuestCondition* const* condition, const char* function) {
    if (condition == nullptr || *condition == nullptr) throw std::invalid_argument(std::string(function) + ": null condition");
    return **condition;
}

bool OwnedByCaller(const GuestMutex& mutex) {
    return mutex.owner.load(std::memory_order_relaxed) == std::this_thread::get_id();
}

}

extern "C" {

int APS5_VABI _Mtx_init_nid_postfix(GuestMutex** mutex, int type) {
    if (mutex == nullptr) throw std::invalid_argument("_Mtx_init: null mutex");
    if ((type & ~(MtxPlain | MtxTry | MtxTimed | MtxRecursive)) != 0) throw std::invalid_argument("_Mtx_init: unknown mutex type " + std::to_string(type));
    *mutex = new (std::nothrow) GuestMutex{};
    if (*mutex == nullptr) return ThrdNomem;
    (*mutex)->type = type;
    return ThrdSuccess;
}

void APS5_VABI _Mtx_destroy_nid_postfix(GuestMutex** mutex) {
    auto& target = MutexOf(mutex, "_Mtx_destroy");
    if (target.owner.load(std::memory_order_relaxed) != std::thread::id{}) throw std::runtime_error("_Mtx_destroy: mutex is locked");
    delete &target;
    *mutex = nullptr;
}

int APS5_VABI _Mtx_lock_nid_postfix(GuestMutex** mutex) {
    auto& target = MutexOf(mutex, "_Mtx_lock");
    if (OwnedByCaller(target)) {
        if ((target.type & MtxRecursive) == 0) throw std::runtime_error("_Mtx_lock: non-recursive mutex already locked by the calling thread");
        ++target.count;
        return ThrdSuccess;
    }
    target.native.lock();
    target.owner.store(std::this_thread::get_id(), std::memory_order_relaxed);
    target.count = 1;
    return ThrdSuccess;
}

int APS5_VABI _Mtx_unlock_nid_postfix(GuestMutex** mutex) {
    auto& target = MutexOf(mutex, "_Mtx_unlock");
    if (!OwnedByCaller(target)) throw std::runtime_error("_Mtx_unlock: mutex is not locked by the calling thread");
    if (--target.count == 0) {
        target.owner.store(std::thread::id{}, std::memory_order_relaxed);
        target.native.unlock();
    }
    return ThrdSuccess;
}

int APS5_VABI _Cnd_init_nid_postfix(GuestCondition** condition) {
    if (condition == nullptr) throw std::invalid_argument("_Cnd_init: null condition");
    *condition = new (std::nothrow) GuestCondition{};
    return *condition != nullptr ? ThrdSuccess : ThrdNomem;
}

void APS5_VABI _Cnd_destroy_nid_postfix(GuestCondition** condition) {
    delete &ConditionOf(condition, "_Cnd_destroy");
    *condition = nullptr;
}

int APS5_VABI _Cnd_wait_nid_postfix(GuestCondition** condition, GuestMutex** mutex) {
    auto& waiting = ConditionOf(condition, "_Cnd_wait");
    auto& target = MutexOf(mutex, "_Cnd_wait");
    if (!OwnedByCaller(target) || target.count != 1) throw std::runtime_error("_Cnd_wait: mutex must be locked exactly once by the calling thread");
    target.count = 0;
    target.owner.store(std::thread::id{}, std::memory_order_relaxed);
    std::unique_lock lock(target.native, std::adopt_lock);
    waiting.native.wait(lock);
    lock.release();
    target.owner.store(std::this_thread::get_id(), std::memory_order_relaxed);
    target.count = 1;
    return ThrdSuccess;
}

int APS5_VABI _Cnd_broadcast_nid_postfix(GuestCondition** condition) {
    ConditionOf(condition, "_Cnd_broadcast").native.notify_all();
    return ThrdSuccess;
}

}
