#include "SlidingWindowLog.h"
#include <chrono>

SlidingWindowLog::SlidingWindowLog(int limit, int windowSecs)
    : limit_(limit), windowSecs_(windowSecs) {}

RateLimitResult SlidingWindowLog::allowRequest(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now          = std::chrono::steady_clock::now();
    auto windowStart  = now - std::chrono::seconds(windowSecs_);

    auto& log = logs_[key];   // creates empty deque if not present

    // Step 1: evict all timestamps older than windowStart
    while (!log.empty() && log.front() <= windowStart) {
        log.pop_front();
    }

    // Step 2: check count BEFORE adding current request
    int currentCount = static_cast<int>(log.size());

    if (currentCount < limit_) {
        // Step 3: allow — record this request's timestamp
        log.push_back(now);
        int remaining = limit_ - static_cast<int>(log.size());
        return { true, remaining, 0,
                 "Allowed (log size: " + std::to_string(log.size()) +
                 "/" + std::to_string(limit_) + ")" };
    }

    // Rejected: calculate when the oldest entry will expire
    auto oldestExpiry = log.front() + std::chrono::seconds(windowSecs_);
    long waitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                      oldestExpiry - now).count();
    waitMs = std::max(waitMs, 1L);

    return { false, 0, waitMs,
             "Rate limited: " + std::to_string(currentCount) +
             " requests in window. Retry after " + std::to_string(waitMs) + "ms" };
}
