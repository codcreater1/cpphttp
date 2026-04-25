#pragma once

#include <string>
#include <functional>
#include <vector>
#include <optional>
#include <unordered_map>
#include <regex>

#include "request.hpp"
#include "response.hpp"

namespace cpphttp {

struct Route {
    std::string                                      method;
    std::string                                      pattern;       // e.g. /users/:id
    std::regex                                       regex;
    std::vector<std::string>                         param_names;
    std::function<void(const Request&, Response&)>   handler;
};

/**
 * @brief Radix-style router with path-parameter support.
 *
 * Supports:
 *   - Exact matches:          /about
 *   - Named parameters:       /users/:id/posts/:postId
 *   - Wildcard suffix:        /static/...
 */
class Router {
public:
    void add(const std::string& method, const std::string& path,
             std::function<void(const Request&, Response&)> handler);

    struct MatchResult {
        const Route*                                        route = nullptr;
        std::unordered_map<std::string, std::string>        params;
    };

    MatchResult match(const std::string& method, const std::string& path) const;

private:
    std::vector<Route> routes_;

    static std::pair<std::regex, std::vector<std::string>>
    compile_pattern(const std::string& pattern);
};

} // namespace cpphttp
