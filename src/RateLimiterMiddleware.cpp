#include "RateLimiterMiddleware.h"

RateLimiterMiddleware::RateLimiterMiddleware(
    std::shared_ptr<RateLimiter> limiter,
    int                           limitPerWindow,
    ApiHandler                    handler)
    : limiter_(std::move(limiter))
    , limitPerWindow_(limitPerWindow)
    , handler_(std::move(handler)) {}

// ── Simulate ByteByteGo Fig 4-13: request → middleware → redis → api ──────
HttpResponse RateLimiterMiddleware::handle(const HttpRequest& req) {
    RateLimitResult result = limiter_->allowRequest(req.clientKey);

    if (result.allowed) {
        // Forward to API handler
        HttpResponse resp = handler_(req);

        // Attach rate-limit informational headers (ByteByteGo: rate limiter headers)
        resp.xRateLimitLimit     = limitPerWindow_;
        resp.xRateLimitRemaining = result.remaining;

        return resp;
    }

    // ── HTTP 429 Too Many Requests (ByteByteGo Fig 4-3) ───────────────────
    HttpResponse blocked;
    blocked.statusCode            = 429;
    blocked.body                  = "Too Many Requests: " + result.reason;
    blocked.xRateLimitLimit       = limitPerWindow_;
    blocked.xRateLimitRemaining   = 0;
    blocked.xRateLimitRetryAfter  = result.retryAfterMs / 1000;

    return blocked;
}
