#pragma once
#include "RateLimiter.h"
#include <unordered_map>
#include <deque>
#include <mutex>
#include <chrono>

// ============================================================
//  SlidingWindowLog.h  —  Sliding Window Log Algorithm
//
//  HOW IT WORKS (ByteByteGo Fig 4–10):
//  • Stores timestamp of every request in a sorted log
//  • On each new request:
//      1. Remove all timestamps older than (now - windowSecs)
//      2. Count remaining timestamps
//      3. If count < limit → allow & append timestamp
//      4. Else → reject (but timestamp NOT added for rejected req)
//
//  PROS:  Very accurate — no boundary burst problem
//  CONS:  Memory heavy: O(requests) per user per window
// ============================================================

class SlidingWindowLog : public RateLimiter {
public:
    // limit      = max requests per window
    // windowSecs = rolling window duration
    SlidingWindowLog(int limit, int windowSecs);

    RateLimitResult allowRequest(const std::string& key) override;
    std::string     name() const override { return "SlidingWindowLog"; }

private:
    using TimePoint = std::chrono::steady_clock::time_point;

    int limit_;
    int windowSecs_;

    // Per-key: ordered queue of request timestamps (oldest front)
    std::unordered_map<std::string, std::deque<TimePoint>> logs_;
    std::mutex mutex_;
};
