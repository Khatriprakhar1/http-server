#pragma once

#include "HttpTypes.hpp"
#include <functional>
#include <map>
#include <string>

using HandlerFn = std::function<HttpResponse(const HttpRequest&)>;

class Router {
public:
    void addRoute(HttpMethod method, const std::string& path, HandlerFn handler);
    HttpResponse route(const HttpRequest& req) const;

private:
    // key: "METHOD /path", e.g. "GET /users"
    std::map<std::string, HandlerFn> m_routes;

    static std::string makeKey(HttpMethod method, const std::string& path);
};
