#include "SlidingWindowCounter.h"
#include <cmath>   // std::floor

SlidingWindowCounter::SlidingWindowCounter(int limit, int windowSecs)
    : limit_(limit), windowSecs_(windowSecs) {}

long long SlidingWindowCounter::currentWindowId() const {
    using namespace std::chrono;
    auto s = duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
    return s / windowSecs_;
}

// How far (0.0–1.0) we are into the current window
double SlidingWindowCounter::elapsedFraction() const {
    using namespace std::chrono;
    auto s = duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
    return static_cast<double>(s % windowSecs_) / static_cast<double>(windowSecs_);
}

RateLimitResult SlidingWindowCounter::allowRequest(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    long long winId = currentWindowId();

    auto it = data_.find(key);
    if (it == data_.end()) {
        data_[key] = { winId, 0, 0 };
        it = data_.find(key);
    }

    WindowData& d = it->second;

    // Rolled into a new window?
    if (d.windowId != winId) {
        d.prevCount = d.currCount;   // current becomes previous
        d.currCount = 0;
        d.windowId  = winId;
    }

    // ByteByteGo formula (Fig 4-11):
    //   approxCount = currCount + prevCount * (1 - elapsedFraction)
    double overlap     = 1.0 - elapsedFraction();
    double approxCount = static_cast<double>(d.currCount) +
                         static_cast<double>(d.prevCount) * overlap;

    if (static_cast<int>(approxCount) < limit_) {
        d.currCount++;
        int remaining = limit_ - static_cast<int>(approxCount) - 1;
        remaining     = std::max(remaining, 0);
        return { true, remaining, 0,
                 "Allowed (approx=" + std::to_string(approxCount) +
                 ", overlap=" + std::to_string(overlap).substr(0,4) + ")" };
    }

    long waitMs = static_cast<long>((1.0 - elapsedFraction()) * windowSecs_ * 1000);
    return { false, 0, waitMs,
             "Rate limited: approx=" + std::to_string(approxCount) +
             " >= limit=" + std::to_string(limit_) };
}
