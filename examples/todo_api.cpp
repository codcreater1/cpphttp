#include <cpphttp.hpp>
#include <iostream>
#include <vector>
#include <mutex>
#include <atomic>
#include <algorithm>

using namespace cpphttp;

// ── In-memory "database" ──────────────────────────────────────────────────────

struct Todo {
    int         id;
    std::string title;
    bool        done;

    std::string to_json() const {
        return R"({"id":)" + std::to_string(id) +
               R"(,"title":")" + title +
               R"(","done":)" + (done ? "true" : "false") + "}";
    }
};

std::vector<Todo>  todos;
std::mutex         todos_mutex;
std::atomic<int>   next_id{1};

// ── Helpers ───────────────────────────────────────────────────────────────────

std::string todos_json() {
    std::string out = "[";
    bool first = true;
    for (auto& t : todos) {
        if (!first) out += ",";
        out += t.to_json();
        first = false;
    }
    return out + "]";
}

// Minimal JSON field extractor (no dependency on a JSON lib)
std::string extract_field(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '"')) ++pos;
    size_t end = json.find_first_of("\",}\n", pos);
    return json.substr(pos, end - pos);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    Server app;

    // Logging middleware
    app.use([](const Request& req, Response& /*res*/) {
        // Nothing to do here — server already logs access
        (void)req;
    });

    // GET /todos
    app.get("/todos", [](const Request& /*req*/, Response& res) {
        std::lock_guard<std::mutex> lock(todos_mutex);
        res.json(todos_json());
    });

    // GET /todos/:id
    app.get("/todos/:id", [](const Request& req, Response& res) {
        int id = std::stoi(req.param("id").value_or("0"));
        std::lock_guard<std::mutex> lock(todos_mutex);
        auto it = std::find_if(todos.begin(), todos.end(),
                               [id](const Todo& t){ return t.id == id; });
        if (it == todos.end()) {
            res.status(404).json(R"({"error":"Todo not found"})");
            return;
        }
        res.json(it->to_json());
    });

    // POST /todos
    app.post("/todos", [](const Request& req, Response& res) {
        std::string title = extract_field(req.body, "title");
        if (title.empty()) {
            res.status(422).json(R"({"error":"title is required"})");
            return;
        }
        Todo t{ next_id++, title, false };
        {
            std::lock_guard<std::mutex> lock(todos_mutex);
            todos.push_back(t);
        }
        res.status(201).json(t.to_json());
    });

    // PUT /todos/:id
    app.put("/todos/:id", [](const Request& req, Response& res) {
        int id = std::stoi(req.param("id").value_or("0"));
        std::lock_guard<std::mutex> lock(todos_mutex);
        auto it = std::find_if(todos.begin(), todos.end(),
                               [id](const Todo& t){ return t.id == id; });
        if (it == todos.end()) {
            res.status(404).json(R"({"error":"Todo not found"})");
            return;
        }
        std::string title = extract_field(req.body, "title");
        std::string done  = extract_field(req.body, "done");
        if (!title.empty()) it->title = title;
        if (!done.empty())  it->done  = (done == "true");
        res.json(it->to_json());
    });

    // DELETE /todos/:id
    app.del("/todos/:id", [](const Request& req, Response& res) {
        int id = std::stoi(req.param("id").value_or("0"));
        std::lock_guard<std::mutex> lock(todos_mutex);
        auto before = todos.size();
        todos.erase(std::remove_if(todos.begin(), todos.end(),
                    [id](const Todo& t){ return t.id == id; }), todos.end());
        if (todos.size() == before) {
            res.status(404).json(R"({"error":"Todo not found"})");
            return;
        }
        res.status(204).send("");
    });

    std::cout << "Todo REST API running on http://localhost:8080\n";
    std::cout << "Try:\n";
    std::cout << "  curl -X POST http://localhost:8080/todos \\\n";
    std::cout << "       -H 'Content-Type: application/json' \\\n";
    std::cout << "       -d '{\"title\":\"Learn C++17\"}'\n";

    app.listen(8080);
}
