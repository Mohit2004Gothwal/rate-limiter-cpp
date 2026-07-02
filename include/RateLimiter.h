#pragma once
#include <string>
#include <chrono>

// ============================================================
//  RateLimiter.h  —  Abstract base for all algorithms
//  ByteByteGo Ch-5: Design a Rate Limiter
// ============================================================

struct RateLimitResult {
    bool   allowed;
    int    remaining;      // tokens / requests left in window
    long   retryAfterMs;   // ms to wait if blocked (0 if allowed)
    std::string reason;
};

class RateLimiter {
public:
    virtual ~RateLimiter() = default;

    // Core: attempt to consume 1 token for the given client key
    virtual RateLimitResult allowRequest(const std::string& key) = 0;

    // Name for logging / monitoring
    virtual std::string name() const = 0;
};
