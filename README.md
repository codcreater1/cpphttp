# ⚡ cpphttp

> A lightweight, dependency-free HTTP/1.1 server built in modern **C++17** — from raw POSIX sockets up.

[![CI](https://github.com/codcreater1/cpphttp/actions/workflows/ci.yml/badge.svg)](https://github.com/codcreater1/cpphttp/actions)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey)]()

```
                                      ___
  ___ _ __  _ __  | |__  | |_  |_|_|_ |_)
 / __| '_ \| '_ \ | '_ \ | __| | |  | |
| (__| |_) | |_) || | | || |_  | |  | |
 \___| .__/| .__/ |_| |_| \__| |_|  |_|
     |_|   |_|
```

`cpphttp` is a learning project that demonstrates how a real HTTP server works — no Boost, no libuv, no framework magic. Just sockets, threads, and clean modern C++.

---

## ✨ Features

| Feature | Details |
|---|---|
| **HTTP/1.1 parser** | Method, path, query string, headers, body |
| **Express-style routing** | `GET /users/:id` with named parameters |
| **Middleware chain** | `app.use(handler)` — runs before every route |
| **Static file serving** | Serve a directory with MIME detection |
| **Thread pool** | One thread per connection (configurable) |
| **Colourised logger** | Access log + structured log levels |
| **Zero dependencies** | Only the C++17 standard library + POSIX |

---

## 📸 Screenshots

**Server running — 12 threads, live access log:**
![terminal](https://github.com/user-attachments/assets/27ce6f12-43bc-4808-998c-374500dfef60)

**Browser output at `localhost:8080`:**
![browser](https://github.com/user-attachments/assets/e053fd27-8cb9-42b6-ae91-213b82349221)

---

## 🚀 Quick Start

### Prerequisites

- CMake ≥ 3.16
- GCC ≥ 9 or Clang ≥ 10 (with C++17 support)
- Linux or macOS

### Build

```bash
git clone https://github.com/codcreater1/cpphttp.git
cd cpphttp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Run the Hello World example

```bash
./build/hello_world
# cpphttp listening on http://0.0.0.0:8080 [8 threads]
```

Open [http://localhost:8080](http://localhost:8080) in your browser or:

```bash
curl http://localhost:8080/api/hello?name=Ada
# {"message":"Hello, Ada!","server":"cpphttp"}

curl http://localhost:8080/api/users/42
# {"user_id":"42","name":"Alice","role":"admin"}
```

---

## 📖 API Reference

### Creating a server

```cpp
#include <cpphttp.hpp>
using namespace cpphttp;

Server app;                    // uses hardware_concurrency() threads
Server app(4);                 // explicit thread count
```

### Routing

```cpp
app.get("/",        handler);
app.post("/items",  handler);
app.put("/items/:id", handler);
app.del("/items/:id", handler);
```

### Handler signature

```cpp
[](const Request& req, Response& res) {
    // Access path params
    std::string id = req.param("id").value_or("0");

    // Access query params  → /search?q=hello
    std::string q  = req.query_param("q").value_or("");

    // Access headers
    std::string ct = req.header("content-type").value_or("");

    // Send responses
    res.json(R"({"ok":true})");
    res.html("<h1>Hello</h1>");
    res.status(201).send("Created");
    res.status(404).json(R"({"error":"not found"})");
}
```

### Middleware

```cpp
// Simple request logger
app.use([](const Request& req, Response& /*res*/) {
    std::cout << req.method << " " << req.path << "\n";
});

// Auth guard (short-circuit by calling res.send())
app.use([](const Request& req, Response& res) {
    if (req.header("x-api-key") != "secret") {
        res.status(401).json(R"({"error":"Unauthorized"})");
    }
});
```

### Static files

```cpp
app.serve_static("/public", "./www");
// GET /public/index.html → serves ./www/index.html
```

### Start & stop

```cpp
app.listen(8080);                   // blocks; default host 0.0.0.0
app.listen(3000, "127.0.0.1");      // custom host

// Graceful shutdown (e.g. from signal handler)
app.stop();
```

---

## 🗂 Project Structure

```
cpphttp/
├── include/
│   ├── cpphttp.hpp          ← Single include
│   └── cpphttp/
│       ├── server.hpp       ← Core server
│       ├── request.hpp      ← HTTP request model
│       ├── response.hpp     ← HTTP response builder
│       ├── router.hpp       ← Path-param router
│       └── logger.hpp       ← Colourised logger
├── src/
│   ├── server.cpp
│   ├── request.cpp
│   ├── response.cpp
│   ├── router.cpp
│   └── logger.cpp
├── examples/
│   ├── hello_world.cpp      ← Getting started
│   └── todo_api.cpp         ← Full CRUD REST API
├── .github/
│   └── workflows/ci.yml     ← GitHub Actions CI
└── CMakeLists.txt
```

---

## 🔍 How It Works

This is the interesting bit — here's the lifecycle of a request:

```
Client
  │
  │  TCP connect
  ▼
accept()          ← main thread blocks here
  │
  │  spawn std::thread
  ▼
recv()            ← read raw bytes from socket
  │
Request::parse()  ← tokenise HTTP/1.1 request line + headers
  │
Middleware chain  ← app.use() handlers run in order
  │
Router::match()   ← regex match path, extract :params
  │
Route handler     ← your lambda runs here
  │
Response::to_raw() ← serialise status + headers + body
  │
send()            ← write to socket
  │
close()
```

---

## 🧪 Running the Todo API example

```bash
./build/todo_api

# Create
curl -X POST http://localhost:8080/todos \
     -H 'Content-Type: application/json' \
     -d '{"title":"Learn C++ sockets"}'

# List
curl http://localhost:8080/todos

# Update
curl -X PUT http://localhost:8080/todos/1 \
     -H 'Content-Type: application/json' \
     -d '{"done":true}'

# Delete
curl -X DELETE http://localhost:8080/todos/1
```

---

## 🛣 Roadmap

- [ ] `keep-alive` connection reuse
- [ ] Chunked transfer encoding
- [ ] HTTP/2 (h2c) upgrade
- [ ] TLS via OpenSSL (opt-in)
- [ ] `multipart/form-data` parser
- [ ] WebSocket upgrade
- [ ] Route groups / sub-routers

---

## 🤝 Contributing

Pull requests are welcome! Please:

1. Fork the repo and create a feature branch
2. Keep changes focused and well-commented
3. Ensure the CI build passes
4. Open a PR with a clear description

---

## 📄 License

MIT © 2024 — see [LICENSE](LICENSE) for details.
