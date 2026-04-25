#include "cpphttp/server.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <filesystem>

namespace cpphttp {

// ── Constructor / Destructor ──────────────────────────────────────────────────

Server::Server(size_t thread_pool_size)
    : pool_size_(thread_pool_size > 0 ? thread_pool_size : 4) {}

Server::~Server() { stop(); }

// ── Fluent config ─────────────────────────────────────────────────────────────

Server& Server::set_backlog(int backlog) { backlog_ = backlog; return *this; }
Server& Server::set_timeout(int seconds) { timeout_s_ = seconds; return *this; }

// ── Route registration ────────────────────────────────────────────────────────

Server& Server::get(const std::string& path, Handler handler) {
    router_.add("GET", path, std::move(handler)); return *this;
}
Server& Server::post(const std::string& path, Handler handler) {
    router_.add("POST", path, std::move(handler)); return *this;
}
Server& Server::put(const std::string& path, Handler handler) {
    router_.add("PUT", path, std::move(handler)); return *this;
}
Server& Server::del(const std::string& path, Handler handler) {
    router_.add("DELETE", path, std::move(handler)); return *this;
}
Server& Server::use(Handler middleware) {
    middlewares_.push_back(std::move(middleware)); return *this;
}

// ── Static file serving ───────────────────────────────────────────────────────

Server& Server::serve_static(const std::string& url_prefix,
                              const std::string& dir_path) {
    router_.add("GET", url_prefix + "/*",
        [dir_path, url_prefix](const Request& req, Response& res) {
            // Strip the prefix to get relative file path
            std::string rel = req.path.substr(url_prefix.size());
            if (rel.empty() || rel == "/") rel = "/index.html";
            std::string full = dir_path + rel;

            // Basic path traversal guard
            namespace fs = std::filesystem;
            fs::path canonical_root = fs::weakly_canonical(dir_path);
            fs::path canonical_file = fs::weakly_canonical(full);
            if (canonical_file.string().find(canonical_root.string()) != 0) {
                res.status(403).send("Forbidden");
                return;
            }
            res.file(full);
        });
    return *this;
}

// ── listen() ─────────────────────────────────────────────────────────────────

void Server::listen(uint16_t port, const std::string& host) {
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));

    // Allow port reuse (avoids TIME_WAIT pain during dev)
    int opt = 1;
    ::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));

    if (::listen(server_fd_, backlog_) < 0)
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));

    running_ = true;
    logger_.info("cpphttp listening on http://" + host + ":" + std::to_string(port)
                 + "  [" + std::to_string(pool_size_) + " threads]");

    accept_loop();
}

// ── stop() ────────────────────────────────────────────────────────────────────

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        ::close(server_fd_);
        server_fd_ = -1;
    }
    for (auto& t : workers_) if (t.joinable()) t.join();
    workers_.clear();
    logger_.info("cpphttp stopped.");
}

// ── Accept loop (main thread) ─────────────────────────────────────────────────

void Server::accept_loop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);

        // Use select() with timeout so we can check running_
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_fd_, &fds);
        timeval tv{1, 0};  // 1 second timeout
        int sel = ::select(server_fd_ + 1, &fds, nullptr, nullptr, &tv);
        if (sel <= 0) continue;

        int client_fd = ::accept(server_fd_,
                                 reinterpret_cast<sockaddr*>(&client_addr),
                                 &client_len);
        if (client_fd < 0) continue;

        // Spawn a thread per connection (simple model; good for learning)
        workers_.emplace_back([this, client_fd]() {
            handle_client(client_fd);
        });

        // Clean up finished threads periodically
        workers_.erase(
            std::remove_if(workers_.begin(), workers_.end(),
                [](std::thread& t) {
                    if (!t.joinable()) return true;
                    return false;
                }),
            workers_.end());
    }
}

// ── Handle a single client connection ────────────────────────────────────────

void Server::handle_client(int client_fd) {
    auto t_start = std::chrono::steady_clock::now();

    // Set receive timeout
    timeval tv{timeout_s_, 0};
    ::setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Read raw data
    constexpr size_t BUF_SIZE = 65536;
    char buf[BUF_SIZE];
    std::string raw;
    ssize_t n;
    while ((n = ::recv(client_fd, buf, sizeof(buf), 0)) > 0) {
        raw.append(buf, static_cast<size_t>(n));
        // Stop when we have the full header (or we've read enough)
        if (raw.find("\r\n\r\n") != std::string::npos) break;
        if (raw.size() > 1024 * 1024) break; // 1 MB safety limit
    }

    Response res;
    std::string method = "?", path = "?";
    int status_code = 500;

    if (!raw.empty()) {
        try {
            Request req = Request::parse(raw);
            method = req.method;
            path   = req.path;
            dispatch(req, res);
            status_code = 200; // approximate
        } catch (const std::exception& e) {
            res.status(400).send("Bad Request: " + std::string(e.what()));
            status_code = 400;
        }
    }

    std::string response = res.to_raw();
    ::send(client_fd, response.data(), response.size(), 0);
    ::close(client_fd);

    auto t_end  = std::chrono::steady_clock::now();
    auto ms     = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
    logger_.access(method, path, status_code, ms);
}

// ── Route dispatch + middleware chain ────────────────────────────────────────

void Server::dispatch(const Request& req_in, Response& res) {
    // Run middlewares in order
    Request req = req_in;
    for (auto& mw : middlewares_) {
        mw(req, res);
        if (res.is_sent()) return;
    }

    // Match route
    auto result = router_.match(req.method, req.path);
    if (!result.route) {
        res.status(404).json(
            R"({"error":"Not Found","path":")" + req.path + R"("})");
        return;
    }

    // Inject path params into request
    req.params = result.params;
    result.route->handler(req, res);

    if (!res.is_sent())
        res.status(204).send("");
}

} // namespace cpphttp
