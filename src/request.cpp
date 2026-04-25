#include "cpphttp/request.hpp"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace cpphttp {

// ── Convenience accessors ────────────────────────────────────────────────────

std::optional<std::string> Request::header(const std::string& name) const {
    // Headers are case-insensitive
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = headers.find(lower);
    if (it == headers.end()) return std::nullopt;
    return it->second;
}

std::optional<std::string> Request::param(const std::string& name) const {
    auto it = params.find(name);
    if (it == params.end()) return std::nullopt;
    return it->second;
}

std::optional<std::string> Request::query_param(const std::string& name) const {
    auto it = query.find(name);
    if (it == query.end()) return std::nullopt;
    return it->second;
}

// ── URL decode ───────────────────────────────────────────────────────────────

std::string Request::url_decode(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '%' && i + 2 < src.size()) {
            int v = 0;
            std::istringstream iss(src.substr(i + 1, 2));
            iss >> std::hex >> v;
            out += static_cast<char>(v);
            i += 2;
        } else if (src[i] == '+') {
            out += ' ';
        } else {
            out += src[i];
        }
    }
    return out;
}

// ── Query-string parser ───────────────────────────────────────────────────────

void Request::parse_query_string(const std::string& qs,
                                 std::unordered_map<std::string, std::string>& out) {
    if (qs.empty()) return;
    std::istringstream ss(qs);
    std::string token;
    while (std::getline(ss, token, '&')) {
        auto eq = token.find('=');
        if (eq == std::string::npos) {
            out[url_decode(token)] = "";
        } else {
            out[url_decode(token.substr(0, eq))] = url_decode(token.substr(eq + 1));
        }
    }
}

// ── HTTP parser ───────────────────────────────────────────────────────────────

Request Request::parse(const std::string& raw) {
    Request req;

    // Split head / body on double CRLF
    auto header_end = raw.find("\r\n\r\n");
    std::string head = (header_end == std::string::npos) ? raw : raw.substr(0, header_end);
    if (header_end != std::string::npos)
        req.body = raw.substr(header_end + 4);

    std::istringstream stream(head);
    std::string line;

    // Request line
    if (!std::getline(stream, line)) throw std::runtime_error("Empty request");
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream rl(line);
    std::string full_path;
    rl >> req.method >> full_path >> req.version;

    // Separate path and query string
    auto q_pos = full_path.find('?');
    if (q_pos == std::string::npos) {
        req.path = full_path;
    } else {
        req.path         = full_path.substr(0, q_pos);
        req.query_string = full_path.substr(q_pos + 1);
        parse_query_string(req.query_string, req.query);
    }

    // Headers (case-insensitive keys)
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key   = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        // Trim leading whitespace from value
        auto start = value.find_first_not_of(" \t");
        if (start != std::string::npos) value = value.substr(start);
        // Lowercase the key
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        req.headers[key] = value;
    }

    return req;
}

} // namespace cpphttp
