#pragma once
#include "RateLimiter.h"
#include <unordered_map>
#include <mutex>
#include <chrono>

// ============================================================
//  TokenBucket.h  —  Token Bucket Algorithm
//
//  HOW IT WORKS (ByteByteGo Fig 4–6):
//  • Bucket holds up to `capacity` tokens
//  • Tokens refill at `refillRate` tokens/second
//  • Each request consumes 1 token
//  • If bucket is empty → reject (HTTP 429)
//
//  PROS:  Allows burst traffic; memory efficient
//  CONS:  Two params hard to tune; needs clock sync in distributed
// ============================================================

class TokenBucket : public RateLimiter {
public:
    // capacity    = max tokens (burst ceiling)
    // refillRate  = tokens added per second
    TokenBucket(int capacity, double refillRate);

    RateLimitResult allowRequest(const std::string& key) override;
    std::string     name() const override { return "TokenBucket"; }

private:
    struct Bucket {
        double   tokens;      // current token count (fractional allowed)
        std::chrono::steady_clock::time_point lastRefill;
    };

    void refill(Bucket& b);   // top-up tokens based on elapsed time

    int    capacity_;
    double refillRate_;        // tokens per second

    std::unordered_map<std::string, Bucket> buckets_;
    std::mutex mutex_;
};
