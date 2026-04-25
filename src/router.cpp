#include "cpphttp/router.hpp"
#include <sstream>
#include <stdexcept>

namespace cpphttp {

// ── Pattern compiler ──────────────────────────────────────────────────────────
// Converts Express-style path (/users/:id/posts/:postId) → std::regex

std::pair<std::regex, std::vector<std::string>>
Router::compile_pattern(const std::string& pattern) {
    std::vector<std::string> names;
    std::string regex_str;
    regex_str.reserve(pattern.size() * 2);
    regex_str += '^';

    size_t i = 0;
    while (i < pattern.size()) {
        char c = pattern[i];
        if (c == ':') {
            // Named parameter: collect until '/' or end
            ++i;
            std::string name;
            while (i < pattern.size() && pattern[i] != '/')
                name += pattern[i++];
            names.push_back(name);
            regex_str += "([^/]+)";
        } else if (c == '*') {
            // Wildcard
            regex_str += "(.*)";
            ++i;
        } else {
            // Escape regex meta-chars
            static const std::string meta = R"(\.+?^${}()|[]/)";
            if (meta.find(c) != std::string::npos)
                regex_str += '\\';
            regex_str += c;
            ++i;
        }
    }
    regex_str += '$';

    return { std::regex(regex_str, std::regex::ECMAScript), names };
}

// ── Route registration ────────────────────────────────────────────────────────

void Router::add(const std::string& method, const std::string& path,
                 std::function<void(const Request&, Response&)> handler) {
    auto [re, names] = compile_pattern(path);
    routes_.push_back({ method, path, re, names, std::move(handler) });
}

// ── Route matching ────────────────────────────────────────────────────────────

Router::MatchResult Router::match(const std::string& method,
                                  const std::string& path) const {
    for (const auto& route : routes_) {
        if (route.method != method && route.method != "*") continue;
        std::smatch m;
        if (!std::regex_match(path, m, route.regex)) continue;

        MatchResult result;
        result.route = &route;
        for (size_t i = 0; i < route.param_names.size(); ++i)
            result.params[route.param_names[i]] = m[i + 1].str();
        return result;
    }
    return {};
}

} // namespace cpphttp
