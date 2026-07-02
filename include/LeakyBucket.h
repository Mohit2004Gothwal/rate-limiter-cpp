#pragma once
#include "RateLimiter.h"
#include <unordered_map>
#include <mutex>
#include <chrono>

// ============================================================
//  LeakyBucket.h  —  Leaky Bucket Algorithm
//
//  HOW IT WORKS (ByteByteGo Fig 4–7):
//  • Each client has a virtual queue of fixed size (capacity)
//  • Requests "leak" out (get processed) at a fixed outflow rate
//  • New request: if queue not full → enqueue; else → drop
//  • We simulate the queue with a fill-level and timestamps
//    (avoids spawning real threads for draining)
//
//  PROS:  Stable outflow; good for payment/billing systems
//  CONS:  Burst requests fill queue fast; old reqs block new ones
//  Used by: Shopify
// ============================================================

class LeakyBucket : public RateLimiter {
public:
    // capacity    = max requests queued
    // outflowRate = requests drained per second
    LeakyBucket(int capacity, double outflowRate);

    RateLimitResult allowRequest(const std::string& key) override;
    std::string     name() const override { return "LeakyBucket"; }

private:
    struct Bucket {
        double   level;       // current fill level (0 = empty)
        std::chrono::steady_clock::time_point lastDrain;
    };

    void drain(Bucket& b);    // compute how much has leaked since lastDrain

    int    capacity_;
    double outflowRate_;      // per second

    std::unordered_map<std::string, Bucket> buckets_;
    std::mutex mutex_;
};
