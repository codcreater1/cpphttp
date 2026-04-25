#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <sstream>

namespace cpphttp {

/**
 * @brief Represents a parsed HTTP/1.1 request.
 */
class Request {
public:
    std::string method;
    std::string path;
    std::string query_string;
    std::string version;
    std::string body;

    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;   // URL path params (:id)
    std::unordered_map<std::string, std::string> query;    // Parsed query string

    // Convenience accessors
    std::optional<std::string> header(const std::string& name) const;
    std::optional<std::string> param(const std::string& name) const;
    std::optional<std::string> query_param(const std::string& name) const;

    std::string remote_addr;

    /**
     * @brief Parse a raw HTTP request string into a Request object.
     * @throws std::runtime_error on malformed input.
     */
    static Request parse(const std::string& raw);

private:
    static void parse_query_string(const std::string& qs,
                                   std::unordered_map<std::string, std::string>& out);
    static std::string url_decode(const std::string& src);
};

} // namespace cpphttp
