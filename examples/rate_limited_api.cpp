#include <cpphttp.hpp>
#include <cpphttp/middleware/rate_limiter.hpp>
#include <chrono>
#include <csignal>
#include <iostream>

using namespace cpphttp;

Server* g_server = nullptr;

void signal_handler(int) {
    std::cout << "\nShutting down...\n";
    if (g_server) g_server->stop();
}

int main() {
    std::signal(SIGINT, signal_handler);

    Server app;
    g_server = &app;

    // 5 requests per 10 seconds, per client IP.
    static middleware::RateLimiter limiter(5, std::chrono::seconds(10));
    app.use(limiter.handler());

    app.get("/api/ping", [](const Request& /*req*/, Response& res) {
        res.json(R"({"message":"pong"})");
    });

    // Try hammering this route more than 5 times in 10s — you'll get a 429.
    std::cout << "Rate-limited server on :8080 (5 req / 10s per IP)\n";
    app.listen(8080);
}
