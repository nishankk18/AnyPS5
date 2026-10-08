#include "prx/libc/include/general/VabiMacros.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <vector>

extern "C" {
int APS5_VABI _Mtx_init_nid_postfix(void** mutex, int type);
void APS5_VABI _Mtx_destroy_nid_postfix(void** mutex);
int APS5_VABI _Mtx_lock_nid_postfix(void** mutex);
int APS5_VABI _Mtx_unlock_nid_postfix(void** mutex);
int APS5_VABI _Cnd_init_nid_postfix(void** condition);
void APS5_VABI _Cnd_destroy_nid_postfix(void** condition);
int APS5_VABI _Cnd_wait_nid_postfix(void** condition, void** mutex);
int APS5_VABI _Cnd_broadcast_nid_postfix(void** condition);
}

namespace {

constexpr int MtxPlain = 0x01;
constexpr int MtxTimed = 0x04;
constexpr int MtxRecursive = 0x100;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::abort();
    }
}

template <typename TException, typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const TException&) {
        return true;
    }
    return false;
}

void CheckPlainMutex() {
    void* mutex = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, MtxPlain) == 0 && mutex != nullptr, "plain mutex init");
    Require(_Mtx_lock_nid_postfix(&mutex) == 0, "plain mutex lock");
    Require(Throws<std::runtime_error>([&] { _Mtx_lock_nid_postfix(&mutex); }), "plain mutex relock must throw");
    Require(Throws<std::runtime_error>([&] { _Mtx_destroy_nid_postfix(&mutex); }), "destroying a locked mutex must throw");
    bool foreignUnlockThrew = false;
    std::thread([&] { foreignUnlockThrew = Throws<std::runtime_error>([&] { _Mtx_unlock_nid_postfix(&mutex); }); }).join();
    Require(foreignUnlockThrew, "unlock by another thread must throw");
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0, "plain mutex unlock");
    Require(Throws<std::runtime_error>([&] { _Mtx_unlock_nid_postfix(&mutex); }), "unlocking an unlocked mutex must throw");
    _Mtx_destroy_nid_postfix(&mutex);
    Require(mutex == nullptr, "destroy clears the handle");
    Require(Throws<std::invalid_argument>([&] { _Mtx_lock_nid_postfix(&mutex); }), "null handle must throw");
    Require(Throws<std::invalid_argument>([&] { _Mtx_init_nid_postfix(&mutex, 0x8); }), "unknown mutex type must throw");
}

void CheckRecursiveMutex() {
    void* mutex = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, MtxTimed | MtxRecursive) == 0, "recursive mutex init");
    Require(_Mtx_lock_nid_postfix(&mutex) == 0 && _Mtx_lock_nid_postfix(&mutex) == 0, "recursive mutex relock");
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0, "recursive mutex first unlock");
    std::atomic<bool> acquired = false;
    std::thread other([&] {
        _Mtx_lock_nid_postfix(&mutex);
        acquired = true;
        _Mtx_unlock_nid_postfix(&mutex);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Require(!acquired, "recursive mutex stays owned until the last unlock");
    Require(_Mtx_unlock_nid_postfix(&mutex) == 0, "recursive mutex last unlock");
    other.join();
    Require(acquired, "recursive mutex released after the last unlock");
    _Mtx_destroy_nid_postfix(&mutex);
}

void CheckMutualExclusion() {
    void* mutex = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, MtxPlain) == 0, "counter mutex init");
    long counter = 0;
    std::vector<std::thread> workers;
    for (int worker = 0; worker < 4; ++worker) {
        workers.emplace_back([&] {
            for (int iteration = 0; iteration < 20000; ++iteration) {
                _Mtx_lock_nid_postfix(&mutex);
                ++counter;
                _Mtx_unlock_nid_postfix(&mutex);
            }
        });
    }
    for (auto& thread : workers) thread.join();
    Require(counter == 80000, "mutex serializes increments");
    _Mtx_destroy_nid_postfix(&mutex);
}

void CheckConditionBroadcast() {
    void* mutex = nullptr;
    void* condition = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, MtxPlain) == 0, "condition mutex init");
    Require(_Cnd_init_nid_postfix(&condition) == 0 && condition != nullptr, "condition init");
    bool ready = false;
    int waiting = 0;
    int woken = 0;
    std::vector<std::thread> waiters;
    for (int index = 0; index < 3; ++index) {
        waiters.emplace_back([&] {
            _Mtx_lock_nid_postfix(&mutex);
            ++waiting;
            while (!ready) Require(_Cnd_wait_nid_postfix(&condition, &mutex) == 0, "condition wait");
            ++woken;
            _Mtx_unlock_nid_postfix(&mutex);
        });
    }
    while (true) {
        _Mtx_lock_nid_postfix(&mutex);
        const bool allWaiting = waiting == 3;
        if (allWaiting) {
            ready = true;
            Require(_Cnd_broadcast_nid_postfix(&condition) == 0, "condition broadcast");
        }
        _Mtx_unlock_nid_postfix(&mutex);
        if (allWaiting) break;
        std::this_thread::yield();
    }
    for (auto& thread : waiters) thread.join();
    Require(woken == 3, "broadcast wakes every waiter");
    Require(_Cnd_broadcast_nid_postfix(&condition) == 0, "broadcast without waiters");
    Require(Throws<std::runtime_error>([&] { _Cnd_wait_nid_postfix(&condition, &mutex); }), "waiting without the mutex must throw");
    _Cnd_destroy_nid_postfix(&condition);
    Require(condition == nullptr, "condition destroy clears the handle");
    _Mtx_destroy_nid_postfix(&mutex);
}

void CheckRecursiveWait() {
    void* mutex = nullptr;
    void* condition = nullptr;
    Require(_Mtx_init_nid_postfix(&mutex, MtxRecursive) == 0 && _Cnd_init_nid_postfix(&condition) == 0, "recursive wait init");
    _Mtx_lock_nid_postfix(&mutex);
    _Mtx_lock_nid_postfix(&mutex);
    Require(Throws<std::runtime_error>([&] { _Cnd_wait_nid_postfix(&condition, &mutex); }), "waiting on a mutex locked twice must throw");
    _Mtx_unlock_nid_postfix(&mutex);
    _Mtx_unlock_nid_postfix(&mutex);
    _Cnd_destroy_nid_postfix(&condition);
    _Mtx_destroy_nid_postfix(&mutex);
}

}

int main() {
    CheckPlainMutex();
    CheckRecursiveMutex();
    CheckMutualExclusion();
    CheckConditionBroadcast();
    CheckRecursiveWait();
    std::puts("Mutex and condition checks passed");
    return 0;
}
