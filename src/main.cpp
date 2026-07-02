#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <string>

#include "RateLimiter.h"
#include "RateLimiterMiddleware.h"
#include "TokenBucket.h"
#include "FixedWindowCounter.h"
#include "SlidingWindowLog.h"
#include "SlidingWindowCounter.h"
#include "LeakyBucket.h"

// ─── Helpers ─────────────────────────────────────────────────────────────────

static void separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

static void sub(const std::string& s) {
    std::cout << "\n── " << s << " ──\n";
}

// Simulate sending N rapid requests through a middleware
static void sendRequests(RateLimiterMiddleware& mw,
                         const std::string&     clientKey,
                         int                    n,
                         int                    delayMs = 0)
{
    for (int i = 1; i <= n; ++i) {
        HttpRequest req { clientKey, "/api/data", "GET", "" };
        HttpResponse resp = mw.handle(req);

        std::cout << "  [" << clientKey << "] req #" << i << " → ";
        resp.print();

        if (delayMs > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}

// ─── Simple API handler (what lives behind the middleware) ────────────────────
static HttpResponse apiHandler(const HttpRequest& req) {
    return { 200, "OK: processed " + req.method + " " + req.endpoint };
}

// ─── Demo 1: Token Bucket ────────────────────────────────────────────────────
void demoTokenBucket() {
    separator("DEMO 1: Token Bucket  (capacity=5, refillRate=1/sec)");
    std::cout << "Burst 8 requests immediately → first 5 pass, rest fail\n";
    std::cout << "Then wait 2s → 2 more tokens refilled\n";

    auto limiter = std::make_shared<TokenBucket>(5, 1.0);  // 5 tokens, 1/sec refill
    RateLimiterMiddleware mw(limiter, 5, apiHandler);

    sub("Burst 8 requests");
    sendRequests(mw, "user_alice", 8);

    sub("Wait 2 seconds...");
    std::this_thread::sleep_for(std::chrono::seconds(2));

    sub("2 more requests (2 tokens refilled)");
    sendRequests(mw, "user_alice", 3);
}

// ─── Demo 2: Fixed Window Counter ────────────────────────────────────────────
void demoFixedWindowCounter() {
    separator("DEMO 2: Fixed Window Counter  (limit=3, window=3s)");
    std::cout << "3 allowed, 4th blocked. Window resets after 3s.\n";

    auto limiter = std::make_shared<FixedWindowCounter>(3, 3);
    RateLimiterMiddleware mw(limiter, 3, apiHandler);

    sub("Send 5 requests");
    sendRequests(mw, "user_bob", 5);

    sub("Wait for window reset (3s)...");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    sub("3 more requests in new window");
    sendRequests(mw, "user_bob", 3);
}

// ─── Demo 3: Sliding Window Log ──────────────────────────────────────────────
void demoSlidingWindowLog() {
    separator("DEMO 3: Sliding Window Log  (limit=3, window=3s)");
    std::cout << "Very accurate: no boundary burst issue.\n";

    auto limiter = std::make_shared<SlidingWindowLog>(3, 3);
    RateLimiterMiddleware mw(limiter, 3, apiHandler);

    sub("Send 5 requests quickly");
    sendRequests(mw, "user_carol", 5);

    sub("Wait 2s, then 2 more (oldest entries expire)");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    sendRequests(mw, "user_carol", 2);
}

// ─── Demo 4: Sliding Window Counter ─────────────────────────────────────────
void demoSlidingWindowCounter() {
    separator("DEMO 4: Sliding Window Counter  (limit=5, window=3s)");
    std::cout << "Hybrid: uses approx = curr + prev * overlap\n";

    auto limiter = std::make_shared<SlidingWindowCounter>(5, 3);
    RateLimiterMiddleware mw(limiter, 5, apiHandler);

    sub("Send 7 requests with 200ms gaps");
    sendRequests(mw, "user_dave", 7, 200);
}

// ─── Demo 5: Leaky Bucket ────────────────────────────────────────────────────
void demoLeakyBucket() {
    separator("DEMO 5: Leaky Bucket  (capacity=4, outflowRate=2/sec)");
    std::cout << "Queue drains at fixed 2/sec. Burst fills queue then blocks.\n";

    auto limiter = std::make_shared<LeakyBucket>(4, 2.0);
    RateLimiterMiddleware mw(limiter, 4, apiHandler);

    sub("Burst 6 requests");
    sendRequests(mw, "user_eve", 6);

    sub("Wait 1.5s (3 slots drain)");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    sub("3 more requests");
    sendRequests(mw, "user_eve", 3);
}

// ─── Demo 6: Multi-client — same limiter, different keys ─────────────────────
void demoMultiClient() {
    separator("DEMO 6: Multi-Client (Token Bucket, capacity=3, rate=1/s)");
    std::cout << "Each client has independent bucket — isolation guaranteed.\n";

    auto limiter = std::make_shared<TokenBucket>(3, 1.0);
    RateLimiterMiddleware mw(limiter, 3, apiHandler);

    sub("client_A sends 4 requests");
    sendRequests(mw, "client_A", 4);

    sub("client_B sends 4 requests (independent bucket — starts full)");
    sendRequests(mw, "client_B", 4);
}

// ─── main ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     Rate Limiter — C++ Implementation                   ║\n";
    std::cout << "║     Based on: ByteByteGo — Design a Rate Limiter        ║\n";
    std::cout << "║     All 5 Algorithms + Middleware Simulation             ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    demoTokenBucket();
    demoFixedWindowCounter();
    demoSlidingWindowLog();
    demoSlidingWindowCounter();
    demoLeakyBucket();
    demoMultiClient();

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  All demos complete.\n";
    std::cout << std::string(60, '=') << "\n\n";
    return 0;
}
