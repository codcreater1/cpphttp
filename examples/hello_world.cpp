#include <cpphttp.hpp>
#include <iostream>
#include <csignal>

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

    // ── Routes ────────────────────────────────────────────────────────────────

    app.get("/", [](const Request& /*req*/, Response& res) {
        res.html(R"(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>cpphttp</title>
  <style>
    body { font-family: monospace; background: #0d1117; color: #58a6ff;
           display: flex; align-items: center; justify-content: center;
           height: 100vh; margin: 0; }
    h1   { font-size: 3rem; letter-spacing: 4px; }
    p    { color: #8b949e; }
  </style>
</head>
<body>
  <div style="text-align:center">
    <h1>⚡ cpphttp</h1>
    <p>A raw C++17 HTTP/1.1 server — running on <b>0 dependencies</b>.</p>
    <p>Try <a href="/api/hello" style="color:#58a6ff">/api/hello</a>
       or <a href="/api/users/42" style="color:#58a6ff">/api/users/42</a></p>
  </div>
</body>
</html>)");
    });

    app.get("/api/hello", [](const Request& req, Response& res) {
        std::string name = req.query_param("name").value_or("World");
        res.json(R"({"message":"Hello, )" + name + R"(!","server":"cpphttp"})");
    });

    app.get("/api/users/:id", [](const Request& req, Response& res) {
        std::string id = req.param("id").value_or("unknown");
        res.json(R"({"user_id":")" + id + R"(","name":"Alice","role":"admin"})");
    });

    app.post("/api/echo", [](const Request& req, Response& res) {
        res.json(R"({"method":"POST","body":)" +
                 (req.body.empty() ? "null" : "\"" + req.body + "\"") + "}");
    });

    app.get("/api/health", [](const Request& /*req*/, Response& res) {
        res.json(R"({"status":"ok","version":"1.0.0"})");
    });

    // ── Start ─────────────────────────────────────────────────────────────────
    app.listen(8080);
}
