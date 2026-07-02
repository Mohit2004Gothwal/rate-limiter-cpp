#include "LeakyBucket.h"
#include <algorithm>   // std::max
#include <cmath>       // std::ceil

LeakyBucket::LeakyBucket(int capacity, double outflowRate)
    : capacity_(capacity), outflowRate_(outflowRate) {}

// ── Drain: compute how many "requests" have leaked since lastDrain ─────────
void LeakyBucket::drain(Bucket& b) {
    auto   now         = std::chrono::steady_clock::now();
    double elapsedSecs = std::chrono::duration<double>(now - b.lastDrain).count();
    double drained     = elapsedSecs * outflowRate_;

    b.level    = std::max(0.0, b.level - drained);
    b.lastDrain = now;
}

// ── Public: attempt to add 1 request to the bucket ─────────────────────────
RateLimitResult LeakyBucket::allowRequest(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (buckets_.find(key) == buckets_.end()) {
        buckets_[key] = { 0.0, std::chrono::steady_clock::now() };
    }

    Bucket& b = buckets_[key];
    drain(b);   // leak out processed requests first

    // Round level up — microseconds of drain between rapid calls
    // shouldn't allow an extra request past the capacity
    int filledSlots = static_cast<int>(std::ceil(b.level));
    if (filledSlots < capacity_) {
        b.level += 1.0;
        int remaining = capacity_ - static_cast<int>(std::ceil(b.level));
        return { true, remaining, 0,
                 "Queued (level=" + std::to_string(static_cast<int>(b.level)) +
                 "/" + std::to_string(capacity_) + ")" };
    }

    // Bucket full — how long until 1 slot frees up?
    double waitSecs = 1.0 / outflowRate_;
    long   waitMs   = static_cast<long>(waitSecs * 1000) + 1;

    return { false, 0, waitMs,
             "Rate limited: bucket full (" + std::to_string(capacity_) +
             "/" + std::to_string(capacity_) + "). Retry after " +
             std::to_string(waitMs) + "ms" };
}
