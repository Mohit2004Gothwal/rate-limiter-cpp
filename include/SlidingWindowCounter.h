#pragma once
#include "RateLimiter.h"
#include <unordered_map>
#include <mutex>
#include <chrono>

// ============================================================
//  SlidingWindowCounter.h  —  Sliding Window Counter Algorithm
//
//  HOW IT WORKS (ByteByteGo Fig 4–11):
//  Hybrid of Fixed Window + Sliding Window Log:
//
//  approx_count = curr_count + prev_count * overlap_fraction
//
//  Where overlap_fraction = portion of the previous window
//  that falls inside the current rolling window.
//
//  Example: limit=7, prev=5, curr=3, position=30% into window
//    overlap = 1 - 0.30 = 0.70
//    approx  = 3 + 5*0.70 = 6.5  → round down → 6  ✓ allow
//
//  PROS:  Smooth traffic; memory efficient (2 counters per key)
//  CONS:  Approximate (0.003% error per Cloudflare experiments)
// ============================================================

class SlidingWindowCounter : public RateLimiter {
public:
    SlidingWindowCounter(int limit, int windowSecs);

    RateLimitResult allowRequest(const std::string& key) override;
    std::string     name() const override { return "SlidingWindowCounter"; }

private:
    struct WindowData {
        long long windowId;    // current window index
        int       currCount;
        int       prevCount;
    };

    long long currentWindowId() const;
    double    elapsedFraction() const;  // how far into current window (0.0–1.0)

    int limit_;
    int windowSecs_;

    std::unordered_map<std::string, WindowData> data_;
    std::mutex mutex_;
};
