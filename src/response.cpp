#include "cpphttp/response.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>

namespace cpphttp {

// ── Status line builder ───────────────────────────────────────────────────────

std::string Response::status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 422: return "Unprocessable Entity";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 503: return "Service Unavailable";
        default:  return "Unknown";
    }
}

// ── MIME type resolver ────────────────────────────────────────────────────────

std::string Response::mime_for_extension(const std::string& ext) {
    static const std::unordered_map<std::string, std::string> mime_map = {
        {".html",  "text/html; charset=utf-8"},
        {".css",   "text/css"},
        {".js",    "application/javascript"},
        {".json",  "application/json"},
        {".png",   "image/png"},
        {".jpg",   "image/jpeg"},
        {".jpeg",  "image/jpeg"},
        {".gif",   "image/gif"},
        {".svg",   "image/svg+xml"},
        {".ico",   "image/x-icon"},
        {".txt",   "text/plain"},
        {".xml",   "application/xml"},
        {".wasm",  "application/wasm"},
    };
    auto it = mime_map.find(ext);
    return (it != mime_map.end()) ? it->second : "application/octet-stream";
}

// ── Fluent setters ────────────────────────────────────────────────────────────

Response& Response::status(int code) {
    status_code_ = code;
    return *this;
}

Response& Response::set(const std::string& header, const std::string& value) {
    headers_[header] = value;
    return *this;
}

Response& Response::type(const std::string& content_type) {
    headers_["Content-Type"] = content_type;
    return *this;
}

// ── Body finalizers ───────────────────────────────────────────────────────────

void Response::send(const std::string& body) {
    body_ = body;
    if (headers_.find("Content-Type") == headers_.end())
        headers_["Content-Type"] = "text/plain; charset=utf-8";
    sent_ = true;
}

void Response::json(const std::string& json_body) {
    headers_["Content-Type"] = "application/json";
    body_ = json_body;
    sent_ = true;
}

void Response::html(const std::string& html_body) {
    headers_["Content-Type"] = "text/html; charset=utf-8";
    body_ = html_body;
    sent_ = true;
}

void Response::send_status(int code) {
    status_code_ = code;
    body_         = status_text(code);
    headers_["Content-Type"] = "text/plain";
    sent_ = true;
}

void Response::redirect(const std::string& url, int code) {
    status_code_ = code;
    headers_["Location"] = url;
    body_ = "";
    sent_ = true;
}

void Response::file(const std::string& file_path) {
    namespace fs = std::filesystem;
    if (!fs::exists(file_path)) {
        status_code_ = 404;
        body_        = "File not found";
        headers_["Content-Type"] = "text/plain";
        sent_ = true;
        return;
    }

    std::ifstream ifs(file_path, std::ios::binary);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    body_ = oss.str();

    std::string ext = fs::path(file_path).extension().string();
    headers_["Content-Type"] = mime_for_extension(ext);
    sent_ = true;
}

// ── Serialiser ────────────────────────────────────────────────────────────────

std::string Response::to_raw() const {
    std::ostringstream out;
    out << "HTTP/1.1 " << status_code_ << " " << status_text(status_code_) << "\r\n";
    out << "Content-Length: " << body_.size() << "\r\n";
    out << "Connection: close\r\n";
    out << "Server: cpphttp/1.0\r\n";
    for (auto& [k, v] : headers_)
        out << k << ": " << v << "\r\n";
    out << "\r\n";
    out << body_;
    return out.str();
}

} // namespace cpphttp
