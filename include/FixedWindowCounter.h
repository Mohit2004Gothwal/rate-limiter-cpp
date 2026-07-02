#pragma once
#include "RateLimiter.h"
#include <unordered_map>
#include <mutex>
#include <chrono>

// ============================================================
//  FixedWindowCounter.h  —  Fixed Window Counter Algorithm
//
//  HOW IT WORKS (ByteByteGo Fig 4–8):
//  • Timeline is divided into fixed windows (e.g. 1 minute)
//  • Each window has its own counter per client key
//  • Counter resets when a new window starts
//  • If counter > limit → reject
//
//  PROS:  Very simple; memory efficient
//  CONS:  Boundary burst: 2× requests possible at window edges
//         (e.g. 5 req at :59 + 5 req at :01 = 10 in 2 seconds)
// ============================================================

class FixedWindowCounter : public RateLimiter {
public:
    // limit      = max requests per window
    // windowSecs = window duration in seconds
    FixedWindowCounter(int limit, int windowSecs);

    RateLimitResult allowRequest(const std::string& key) override;
    std::string     name() const override { return "FixedWindowCounter"; }

private:
    struct Window {
        long long windowId;   // which window (epoch / windowSecs)
        int       count;
    };

    long long currentWindowId() const;

    int limit_;
    int windowSecs_;

    std::unordered_map<std::string, Window> windows_;
    std::mutex mutex_;
};
