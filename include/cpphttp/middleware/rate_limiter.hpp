#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../request.hpp"
#include "../response.hpp"

namespace cpphttp::middleware {

/**
 * @brief Per-client token-bucket rate limiter.
 *
 * Usage:
 *   cpphttp::middleware::RateLimiter limiter(100, std::chrono::seconds(60));
 *   app.use(limiter.handler());
 *
 * Buckets are keyed by Request::remote_addr. Each bucket refills
 * continuously at max_requests / window, capped at max_requests.
 * Thread-safe: guarded by a single mutex, sized for thread-per-connection
 * servers with moderate concurrency.
 */
class RateLimiter {
public:
    RateLimiter(std::size_t max_requests, std::chrono::steady_clock::duration window)
        : max_requests_(max_requests), window_(window) {}

    using Handler = std::function<void(const Request&, Response&)>;

    Handler handler() {
        return [this](const Request& req, Response& res) { this->check(req, res); };
    }

    void check(const Request& req, Response& res) {
        const auto& key = req.remote_addr;
        const auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = buckets_.find(key);
        if (it == buckets_.end()) {
            it = buckets_.emplace(key, Bucket{static_cast<double>(max_requests_), now}).first;
        }
        auto& bucket = it->second;

        if (bucket.tokens < static_cast<double>(max_requests_)) {
            const double elapsed_s =
                std::chrono::duration<double>(now - bucket.last_refill).count();
            const double refill_rate =
                static_cast<double>(max_requests_) /
                std::chrono::duration<double>(window_).count();
            bucket.tokens =
                std::min(static_cast<double>(max_requests_),
                         bucket.tokens + elapsed_s * refill_rate);
        }
        bucket.last_refill = now;

        if (bucket.tokens < 1.0) {
            const double refill_rate =
                static_cast<double>(max_requests_) /
                std::chrono::duration<double>(window_).count();
            const int retry_after_s =
                static_cast<int>(std::ceil((1.0 - bucket.tokens) / refill_rate));
            res.set("Retry-After", std::to_string(retry_after_s))
                .status(429)
                .json(R"({"error":"Too Many Requests"})");
            return;
        }

        bucket.tokens -= 1.0;
    }

private:
    struct Bucket {
        double tokens = 0.0;
        std::chrono::steady_clock::time_point last_refill = std::chrono::steady_clock::now();
    };

    std::size_t                                    max_requests_;
    std::chrono::steady_clock::duration            window_;
    std::unordered_map<std::string, Bucket>        buckets_;
    std::mutex                                      mutex_;
};

} // namespace cpphttp::middleware
