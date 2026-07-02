#include "TokenBucket.h"
#include <algorithm>   // std::min
#include <cmath>       // std::floor

TokenBucket::TokenBucket(int capacity, double refillRate)
    : capacity_(capacity), refillRate_(refillRate) {}

// ── Private: top-up tokens based on time elapsed since last refill ─────────
void TokenBucket::refill(Bucket& b) {
    auto now     = std::chrono::steady_clock::now();
    double elapsedSecs = std::chrono::duration<double>(now - b.lastRefill).count();

    // Add tokens proportional to elapsed time
    double newTokens = elapsedSecs * refillRate_;
    b.tokens     = std::min(static_cast<double>(capacity_), b.tokens + newTokens);
    b.lastRefill = now;
}

// ── Public: attempt to consume 1 token ────────────────────────────────────
RateLimitResult TokenBucket::allowRequest(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Create bucket on first request for this key
    if (buckets_.find(key) == buckets_.end()) {
        buckets_[key] = { static_cast<double>(capacity_),
                          std::chrono::steady_clock::now() };
    }

    Bucket& b = buckets_[key];
    refill(b);   // top-up first

    if (b.tokens >= 1.0) {
        b.tokens -= 1.0;
        int remaining = static_cast<int>(std::floor(b.tokens));

        return { true, remaining, 0,
                 "Request allowed (tokens left: " + std::to_string(remaining) + ")" };
    }

    // Calculate wait time until 1 token is available
    double waitSecs   = (1.0 - b.tokens) / refillRate_;
    long   waitMs     = static_cast<long>(waitSecs * 1000) + 1;

    return { false, 0, waitMs,
             "Rate limited: bucket empty. Retry after " + std::to_string(waitMs) + "ms" };
}
