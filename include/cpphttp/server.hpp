#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <stdexcept>

#include "request.hpp"
#include "response.hpp"
#include "router.hpp"
#include "logger.hpp"

namespace cpphttp {

/**
 * @brief Core HTTP/1.1 server built on raw POSIX sockets.
 *
 * Usage:
 *   Server app;
 *   app.get("/hello", [](const Request& req, Response& res) {
 *       res.send("Hello, World!");
 *   });
 *   app.listen(8080);
 */
class Server {
public:
    using Handler = std::function<void(const Request&, Response&)>;

    explicit Server(size_t thread_pool_size = std::thread::hardware_concurrency());
    ~Server();

    // Route registration
    Server& get(const std::string& path, Handler handler);
    Server& post(const std::string& path, Handler handler);
    Server& put(const std::string& path, Handler handler);
    Server& del(const std::string& path, Handler handler);

    // Middleware support
    Server& use(Handler middleware);

    // Static file serving
    Server& serve_static(const std::string& url_prefix, const std::string& dir_path);

    // Start listening
    void listen(uint16_t port, const std::string& host = "0.0.0.0");

    // Graceful shutdown
    void stop();

    // Fluent config
    Server& set_backlog(int backlog);
    Server& set_timeout(int seconds);

private:
    void accept_loop();
    void handle_client(int client_fd);
    void dispatch(const Request& req, Response& res);

    int                     server_fd_   = -1;
    std::atomic<bool>       running_     {false};
    size_t                  pool_size_;
    int                     backlog_     = 128;
    int                     timeout_s_  = 30;

    Router                  router_;
    std::vector<Handler>    middlewares_;
    std::vector<std::thread> workers_;
    Logger                  logger_;
};

} // namespace cpphttp
