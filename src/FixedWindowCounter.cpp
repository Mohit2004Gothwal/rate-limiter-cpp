#include "FixedWindowCounter.h"

FixedWindowCounter::FixedWindowCounter(int limit, int windowSecs)
    : limit_(limit), windowSecs_(windowSecs) {}

long long FixedWindowCounter::currentWindowId() const {
    using namespace std::chrono;
    auto epochMs = duration_cast<seconds>(
        steady_clock::now().time_since_epoch()).count();
    return epochMs / windowSecs_;   // integer division → window index
}

RateLimitResult FixedWindowCounter::allowRequest(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    long long winId = currentWindowId();

    // Create or reset window if we're in a new time window
    auto it = windows_.find(key);
    if (it == windows_.end() || it->second.windowId != winId) {
        windows_[key] = { winId, 0 };
        it = windows_.find(key);
    }

    Window& w = it->second;

    if (w.count < limit_) {
        w.count++;
        int remaining = limit_ - w.count;
        return { true, remaining, 0,
                 "Allowed (window count: " + std::to_string(w.count) + "/" +
                 std::to_string(limit_) + ")" };
    }

    // Next window starts at (winId+1) * windowSecs_ seconds from epoch
    // We give a rough retry estimate of windowSecs_
    long retryMs = static_cast<long>(windowSecs_) * 1000L;
    return { false, 0, retryMs,
             "Rate limited: window counter full (" + std::to_string(limit_) +
             "/" + std::to_string(limit_) + "). Retry after " +
             std::to_string(windowSecs_) + "s" };
}
