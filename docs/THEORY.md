# Rate Limiter — Theory & Design
## Based on ByteByteGo: Design a Rate Limiter (Chapter 5)

---

## 1. What is a Rate Limiter?

A rate limiter controls how many requests a client can send to a server
within a time window. Excess requests are blocked with HTTP 429.

**Real examples:**
- Twitter: 300 tweets per 3 hours
- Google Docs API: 300 reads per user per 60s
- Stripe: uses Token Bucket per API key

---

## 2. Why Do We Need It?

| Benefit | Explanation |
|---------|-------------|
| **DoS protection** | Blocks malicious/accidental request floods |
| **Cost control** | Reduces bills for pay-per-call APIs |
| **Fair usage** | Prevents one client from starving others |
| **Server stability** | Protects backend from bot traffic |

---

## 3. Where to Place It?

```
Client → [Rate Limiter Middleware] → API Servers
                    ↕
                 Redis Cache
                    ↕
             Rules (disk/config)
```

**Options:**
- **Client-side:** Unreliable — can be forged
- **Server-side:** Full control, but couples to app
- **API Gateway:** Best for microservices (Nginx, Envoy, Kong)

---

## 4. The 5 Algorithms

### 4.1 Token Bucket
```
Bucket [■■■□□] capacity=5, tokens=3
Each request: consume 1 token
Refill: +1 token/second (up to capacity)
If tokens=0: reject (429)
```
- **Use when:** You need burst tolerance (Amazon, Stripe)
- **Params:** bucket_size, refill_rate
- **Pros:** Simple, allows burst
- **Cons:** Hard to tune; clock sync needed in distributed

### 4.2 Leaky Bucket
```
Requests → [queue: ■■■■□] → drain at fixed rate → API
If queue full → drop
```
- **Use when:** You need a stable output rate (payments, Shopify)
- **Params:** queue_size, outflow_rate
- **Pros:** Smooth output; prevents spikes
- **Cons:** Old requests block new ones during bursts

### 4.3 Fixed Window Counter
```
|  window 1  |  window 2  |  window 3  |
| count: 3/3 | count: 1/3 | count: 3/3 |
             ↑ resets here
```
- **Use when:** Simple per-minute/hour limits
- **Params:** limit, window_size
- **Pros:** Very simple; memory efficient
- **Cons:** Boundary burst: 2× requests at window edge

### 4.4 Sliding Window Log
```
Log: [1:00:01, 1:00:30, 1:00:50]  ← actual timestamps
On request at 1:01:40:
  Remove entries < 1:00:40 → log = [1:00:50]
  count=1 < limit=2 → allow, add 1:01:40
```
- **Use when:** Strict accuracy required
- **Params:** limit, window_size
- **Pros:** Most accurate; no boundary issue
- **Cons:** Memory heavy O(requests)

### 4.5 Sliding Window Counter
```
approx = curr_count + prev_count × (1 - elapsed_fraction)
Example: curr=3, prev=5, elapsed=30% → approx = 3 + 5×0.70 = 6.5
```
- **Use when:** Balance of accuracy and efficiency (Cloudflare)
- **Params:** limit, window_size
- **Pros:** Memory efficient; smooth; 0.003% error per Cloudflare
- **Cons:** Approximate only

---

## 5. Algorithm Comparison

| Algorithm | Memory | Accuracy | Burst | Complexity |
|-----------|--------|----------|-------|------------|
| Token Bucket | O(1) | High | ✅ Yes | Low |
| Leaky Bucket | O(n queue) | High | ❌ No | Medium |
| Fixed Window | O(1) | Medium | ⚠️ Edge | Very Low |
| Sliding Log | O(requests) | Very High | ❌ No | Low |
| Sliding Counter | O(1) | High | ⚠️ Approx | Low |

---

## 6. Distributed Rate Limiting

### The Race Condition Problem (ByteByteGo Fig 4-14)
```
Thread A reads counter = 3
Thread B reads counter = 3
Thread A writes counter = 4  ← lost update!
Thread B writes counter = 4  ← should be 5
```

### Solutions
1. **Lua Scripts in Redis** — atomic read-modify-write
2. **Redis MULTI/EXEC** — optimistic locking
3. **Redis sorted sets** — for sliding window log

### Synchronization (ByteByteGo Fig 4-15, 4-16)
- ❌ Sticky sessions: not scalable
- ✅ Centralized Redis: all limiters share one store
- ✅ Eventual consistency sync: local counters, periodic sync

---

## 7. HTTP Headers (ByteByteGo: Rate Limiter Headers)

```
X-RateLimit-Limit:      100    ← max requests per window
X-RateLimit-Remaining:  42     ← how many left
X-RateLimit-Retry-After: 30    ← seconds to wait if 429
```

---

## 8. Hard vs Soft Rate Limiting

| Type | Behaviour |
|------|-----------|
| **Hard** | Never exceed the threshold (payments) |
| **Soft** | Allow brief bursts over threshold (social feeds) |

---

## 9. OSI Layer Considerations

- **Layer 7 (Application):** HTTP rate limiting (this project)
- **Layer 3 (Network):** IP-level via iptables / firewall rules
- **Layer 4 (Transport):** TCP connection limits

---

## 10. Production Architecture (ByteByteGo Fig 4-13)

```
[Rules on disk]
      ↓ (Workers pull periodically)
[Rules Cache]
      ↓
Client → Rate Limiter Middleware → Redis (counters)
              ↓              ↓
          Allowed         Blocked
              ↓              ↓
         API Servers      429 + Queue (retry later)
```
