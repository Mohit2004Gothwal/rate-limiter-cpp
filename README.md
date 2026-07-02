# Rate Limiter in C++
## ByteByteGo — System Design Interview: Design a Rate Limiter (Chapter 5)

---

## Project Structure

```
rate_limiter/
├── CMakeLists.txt              ← Build configuration
├── README.md                   ← This file
├── config/
│   └── rules.yaml              ← Rate limiting rules (ByteByteGo format)
├── docs/
│   └── THEORY.md               ← Full theory + algorithm explanations
├── include/
│   ├── RateLimiter.h           ← Abstract base interface
│   ├── TokenBucket.h           ← Algorithm 1
│   ├── LeakyBucket.h           ← Algorithm 2
│   ├── FixedWindowCounter.h    ← Algorithm 3
│   ├── SlidingWindowLog.h      ← Algorithm 4
│   ├── SlidingWindowCounter.h  ← Algorithm 5
│   └── RateLimiterMiddleware.h ← HTTP middleware simulation
├── src/
│   ├── TokenBucket.cpp
│   ├── LeakyBucket.cpp
│   ├── FixedWindowCounter.cpp
│   ├── SlidingWindowLog.cpp
│   ├── SlidingWindowCounter.cpp
│   ├── RateLimiterMiddleware.cpp
│   └── main.cpp                ← Demo: all 5 algorithms
└── tests/
    └── test_all.cpp            ← Unit tests
```

---

## Step-by-Step Build & Run

### Prerequisites
- GCC 8+ or Clang 7+  (C++17 required)
- CMake 3.14+

### Step 1 — Clone / enter the project
```bash
cd rate_limiter
```

### Step 2 — Create build directory
```bash
mkdir build && cd build
```

### Step 3 — Configure with CMake
```bash
cmake ..
```

### Step 4 — Build everything
```bash
cmake --build .
```

### Step 5 — Run the demo
```bash
./rate_limiter_demo
```

### Step 6 — Run unit tests
```bash
./rate_limiter_tests
# or via CTest:
ctest --output-on-failure
```

---

## Algorithms Implemented

| # | Algorithm | C++ Class | ByteByteGo Fig |
|---|-----------|-----------|----------------|
| 1 | Token Bucket | `TokenBucket` | Fig 4-4, 4-5, 4-6 |
| 2 | Leaky Bucket | `LeakyBucket` | Fig 4-7 |
| 3 | Fixed Window Counter | `FixedWindowCounter` | Fig 4-8, 4-9 |
| 4 | Sliding Window Log | `SlidingWindowLog` | Fig 4-10 |
| 5 | Sliding Window Counter | `SlidingWindowCounter` | Fig 4-11 |

---

## How to Use in Your Own Code

```cpp
#include "TokenBucket.h"
#include "RateLimiterMiddleware.h"

// 1. Choose an algorithm
auto limiter = std::make_shared<TokenBucket>(
    100,   // capacity: 100 tokens
    10.0   // refill:   10 tokens/second
);

// 2. Wrap in middleware with your API handler
RateLimiterMiddleware mw(limiter, 100, [](const HttpRequest& req) {
    return HttpResponse{ 200, "OK" };
});

// 3. Handle incoming requests
HttpRequest req { "user_123", "/api/data", "GET", "" };
HttpResponse resp = mw.handle(req);

if (resp.statusCode == 429) {
    // Rate limited — check X-RateLimit-Retry-After header
}
```

---

## Interview Talking Points

1. **Start with Token Bucket** — it's what Amazon and Stripe use
2. **Mention Redis** — for distributed rate limiting across servers
3. **Discuss race conditions** — Lua scripts for atomic operations
4. **HTTP headers** — X-RateLimit-Limit, -Remaining, -Retry-After
5. **Hard vs Soft** — strict (payments) vs lenient (social feeds)
6. **Fail Open** — if Redis goes down, allow traffic (availability > accuracy)
