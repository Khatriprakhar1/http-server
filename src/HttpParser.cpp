#include "HttpParser.hpp"
#include <sstream>
#include <stdexcept>
#include <algorithm>

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

HttpMethod stringToMethod(const std::string& str) {
    if (str == "GET") return HttpMethod::GET;
    if (str == "POST") return HttpMethod::POST;
    if (str == "PUT") return HttpMethod::PUT;
    if (str == "DELETE") return HttpMethod::DELETE;
    if (str == "PATCH") return HttpMethod::PATCH;
    return HttpMethod::UNKNOWN;
}

std::string methodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        default: return "UNKNOWN";
    }
}

std::string HttpResponse::toString() const {
    std::ostringstream out;
    out << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    for (const auto& [key, value] : headers)
        out << key << ": " << value << "\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "\r\n" << body;
    return out.str();
}

void HttpParser::parseRequestLine(const std::string& line, HttpRequest& req) {
    std::istringstream iss(line);
    std::string methodStr;
    iss >> methodStr >> req.path >> req.version;
    if (methodStr.empty() || req.path.empty())
        throw std::runtime_error("Malformed request line");
    req.method = stringToMethod(methodStr);
}

void HttpParser::parseHeaderLine(const std::string& line, HttpRequest& req) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) return;
    std::string key = trim(line.substr(0, colonPos));
    std::string value = trim(line.substr(colonPos + 1));
    req.headers[key] = value;
}

HttpRequest HttpParser::parse(const std::string& rawRequest) {
    HttpRequest req;
    std::istringstream stream(rawRequest);
    std::string line;

    if (!std::getline(stream, line))
        throw std::runtime_error("Empty request");
    parseRequestLine(trim(line), req);

    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) break; // blank line = end of headers
        parseHeaderLine(line, req);
    }

    std::ostringstream bodyStream;
    bodyStream << stream.rdbuf();
    req.body = bodyStream.str();

    return req;
}