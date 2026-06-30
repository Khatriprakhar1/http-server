#pragma once

#include "HttpTypes.hpp"
#include <string>

class HttpParser {
public:
    static HttpRequest parse(const std::string& rawRequest);

private:
    static void parseRequestLine(const std::string& line, HttpRequest& req);
    static void parseHeaderLine(const std::string& line, HttpRequest& req);
};
