#pragma once
#include "RateLimiter.h"
#include <memory>
#include <string>
#include <functional>
#include <iostream>

// ============================================================
//  RateLimiterMiddleware.h
//
//  Simulates the middleware layer described in ByteByteGo Fig 4–2.
//  Wraps any RateLimiter and acts as a gatekeeper:
//
//    Client → [RateLimiterMiddleware] → API Server
//                     ↕
//                   Redis (simulated in-process)
//
//  In production this would be an Nginx/Envoy plugin or a
//  standalone service. Here it wraps a handler function.
// ============================================================

// Simulates an HTTP request
struct HttpRequest {
    std::string clientKey;   // user_id, IP, or API key
    std::string endpoint;
    std::string method;
    std::string body;
};

// Simulates an HTTP response
struct HttpResponse {
    int         statusCode;   // 200, 429, etc.
    std::string body;

    // Rate-limit headers (ByteByteGo: Rate limiter headers section)
    int  xRateLimitRemaining  = -1;
    int  xRateLimitLimit      = -1;
    long xRateLimitRetryAfter = -1;   // seconds

    void print() const {
        std::cout << "  HTTP " << statusCode << "\n";
        if (xRateLimitLimit >= 0)
            std::cout << "  X-RateLimit-Limit:      " << xRateLimitLimit << "\n";
        if (xRateLimitRemaining >= 0)
            std::cout << "  X-RateLimit-Remaining:  " << xRateLimitRemaining << "\n";
        if (xRateLimitRetryAfter >= 0)
            std::cout << "  X-RateLimit-Retry-After:" << xRateLimitRetryAfter << "s\n";
        std::cout << "  Body: " << body << "\n";
    }
};

using ApiHandler = std::function<HttpResponse(const HttpRequest&)>;

class RateLimiterMiddleware {
public:
    RateLimiterMiddleware(std::shared_ptr<RateLimiter> limiter,
                          int                           limitPerWindow,
                          ApiHandler                    handler);

    HttpResponse handle(const HttpRequest& req);

private:
    std::shared_ptr<RateLimiter> limiter_;
    int                          limitPerWindow_;
    ApiHandler                   handler_;
};
