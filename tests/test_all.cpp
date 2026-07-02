#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

#include "TokenBucket.h"
#include "FixedWindowCounter.h"
#include "SlidingWindowLog.h"
#include "SlidingWindowCounter.h"
#include "LeakyBucket.h"

// ─── Minimal test framework ──────────────────────────────────────────────────
static int passed = 0, failed = 0;

#define EXPECT_TRUE(expr) \
    if (!(expr)) { \
        std::cout << "  ✗ FAIL: " #expr " (line " << __LINE__ << ")\n"; \
        ++failed; \
    } else { \
        std::cout << "  ✓ PASS: " #expr "\n"; \
        ++passed; \
    }

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

// ─── Token Bucket tests ──────────────────────────────────────────────────────
void testTokenBucket() {
    std::cout << "\n[TokenBucket]\n";

    TokenBucket tb(3, 1.0);  // capacity=3, refill=1/sec

    // Burst: first 3 should pass
    EXPECT_TRUE(tb.allowRequest("u1").allowed)
    EXPECT_TRUE(tb.allowRequest("u1").allowed)
    EXPECT_TRUE(tb.allowRequest("u1").allowed)

    // 4th should fail (bucket empty)
    EXPECT_FALSE(tb.allowRequest("u1").allowed)

    // Different client: independent bucket, full
    EXPECT_TRUE(tb.allowRequest("u2").allowed)

    // Wait 2s → 2 tokens refilled
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));
    EXPECT_TRUE(tb.allowRequest("u1").allowed)
    EXPECT_TRUE(tb.allowRequest("u1").allowed)
    EXPECT_FALSE(tb.allowRequest("u1").allowed)  // only 2 refilled
}

// ─── Fixed Window Counter tests ──────────────────────────────────────────────
void testFixedWindowCounter() {
    std::cout << "\n[FixedWindowCounter]\n";

    FixedWindowCounter fw(3, 5);  // limit=3, window=5s

    EXPECT_TRUE(fw.allowRequest("u1").allowed)
    EXPECT_TRUE(fw.allowRequest("u1").allowed)
    EXPECT_TRUE(fw.allowRequest("u1").allowed)
    EXPECT_FALSE(fw.allowRequest("u1").allowed)  // 4th blocked

    // Different key: independent window
    EXPECT_TRUE(fw.allowRequest("u2").allowed)

    // Result should carry remaining count
    FixedWindowCounter fw2(5, 10);
    auto r = fw2.allowRequest("x");
    EXPECT_TRUE(r.allowed)
    EXPECT_TRUE(r.remaining == 4)
}

// ─── Sliding Window Log tests ────────────────────────────────────────────────
void testSlidingWindowLog() {
    std::cout << "\n[SlidingWindowLog]\n";

    SlidingWindowLog sl(3, 3);  // limit=3, window=3s

    EXPECT_TRUE(sl.allowRequest("u1").allowed)
    EXPECT_TRUE(sl.allowRequest("u1").allowed)
    EXPECT_TRUE(sl.allowRequest("u1").allowed)
    EXPECT_FALSE(sl.allowRequest("u1").allowed)

    // Wait for window to expire → all 3 entries evicted
    std::this_thread::sleep_for(std::chrono::milliseconds(3100));
    EXPECT_TRUE(sl.allowRequest("u1").allowed)
}

// ─── Sliding Window Counter tests ────────────────────────────────────────────
void testSlidingWindowCounter() {
    std::cout << "\n[SlidingWindowCounter]\n";

    SlidingWindowCounter sc(4, 5);  // limit=4, window=5s

    EXPECT_TRUE(sc.allowRequest("u1").allowed)
    EXPECT_TRUE(sc.allowRequest("u1").allowed)
    EXPECT_TRUE(sc.allowRequest("u1").allowed)
    EXPECT_TRUE(sc.allowRequest("u1").allowed)
    // 5th: approx count ≥ 4 → blocked
    EXPECT_FALSE(sc.allowRequest("u1").allowed)

    // Independent client
    EXPECT_TRUE(sc.allowRequest("u2").allowed)
}

// ─── Leaky Bucket tests ──────────────────────────────────────────────────────
void testLeakyBucket() {
    std::cout << "\n[LeakyBucket]\n";

    LeakyBucket lb(3, 10.0);  // capacity=3, outflow=10/sec

    EXPECT_TRUE(lb.allowRequest("u1").allowed)   // level becomes 1
    EXPECT_TRUE(lb.allowRequest("u1").allowed)   // level becomes 2
    EXPECT_TRUE(lb.allowRequest("u1").allowed)   // level becomes 3
    // 4th: int(level)=3 >= capacity=3 → blocked
    EXPECT_FALSE(lb.allowRequest("u1").allowed)

    // Wait 300ms → 3 slots drained (10/sec * 0.3s = 3)
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_TRUE(lb.allowRequest("u1").allowed)
}

// ─── Retry-After tests ───────────────────────────────────────────────────────
void testRetryAfter() {
    std::cout << "\n[RetryAfter headers]\n";

    TokenBucket tb(1, 1.0);
    tb.allowRequest("u1");  // consume the 1 token

    auto r = tb.allowRequest("u1");
    EXPECT_FALSE(r.allowed)
    EXPECT_TRUE(r.retryAfterMs > 0)
    std::cout << "  retryAfterMs = " << r.retryAfterMs << "\n";

    SlidingWindowLog sl(1, 2);
    sl.allowRequest("u2");
    auto r2 = sl.allowRequest("u2");
    EXPECT_FALSE(r2.allowed)
    EXPECT_TRUE(r2.retryAfterMs > 0)
}

// ─── main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║  Rate Limiter — Unit Tests               ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";

    testTokenBucket();
    testFixedWindowCounter();
    testSlidingWindowLog();
    testSlidingWindowCounter();
    testLeakyBucket();
    testRetryAfter();

    std::cout << "\n─────────────────────────────────────────\n";
    std::cout << "  Passed: " << passed << "  Failed: " << failed << "\n";
    std::cout << "─────────────────────────────────────────\n\n";

    return failed == 0 ? 0 : 1;
}
