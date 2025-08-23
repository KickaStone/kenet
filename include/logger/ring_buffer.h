//
// Created by jijuncheng on 8/17/25.
//

#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <array>
#include <atomic>
#include <type_traits>


// SPSC ring
template <typename T, size_t CapacityPow2>
class SpscRing {
    static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0, "Capacity must be power of 2");
    static_assert(std::is_trivially_copyable<T> :: value, "T must be trivially copyable");
    static constexpr size_t MASK = CapacityPow2 - 1;
public:
    bool try_push(const T& v) {
        auto w = write_.load(std::memory_order_relaxed);
        auto n = w + 1;
        if (n==read_.load(std::memory_order_acquire)) return false; // full
        buf_[w & MASK] = v;
        write_.store(n, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out) {
        auto r = read_.load(std::memory_order_relaxed);
        if (r == write_.load(std::memory_order_acquire)) return false; // empty
        out = buf_[r & MASK];
        read_.store(r + 1, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return read_.load(std::memory_order_acquire) == write_.load(std::memory_order_acquire);
    }
private:
    alignas(64) std::atomic<size_t> write_{0};
    alignas(64) std::atomic<size_t> read_{0 };
    alignas(64) std::array<T, CapacityPow2> buf_{};
};


#endif //RING_BUFFER_H
