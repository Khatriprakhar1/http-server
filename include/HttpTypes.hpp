#pragma once

#include <string>
#include <unordered_map>

enum class HttpMethod {
    GET, POST, PUT, DELETE, PATCH, UNKNOWN
};

struct HttpRequest {
    HttpMethod method = HttpMethod::UNKNOWN;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int statusCode = 200;
    std::string statusText = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string toString() const;
};

HttpMethod stringToMethod(const std::string& str);
std::string methodToString(HttpMethod method);
