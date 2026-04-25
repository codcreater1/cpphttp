#pragma once

#include <string>
#include <unordered_map>
#include <sstream>

namespace cpphttp {

/**
 * @brief Builds and serialises an HTTP/1.1 response.
 */
class Response {
public:
    // Status setters
    Response& status(int code);
    Response& set(const std::string& header, const std::string& value);
    Response& type(const std::string& content_type);

    // Body setters (finalise response)
    void send(const std::string& body);
    void json(const std::string& json_body);
    void html(const std::string& html_body);
    void file(const std::string& file_path);
    void redirect(const std::string& url, int code = 302);
    void send_status(int code);

    // Serialise to raw bytes ready for the socket
    std::string to_raw() const;

    bool is_sent() const { return sent_; }

private:
    int                                            status_code_ = 200;
    std::string                                    body_;
    std::unordered_map<std::string, std::string>   headers_;
    bool                                           sent_ = false;

    static std::string status_text(int code);
    static std::string mime_for_extension(const std::string& ext);
};

} // namespace cpphttp
