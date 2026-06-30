#include "Router.hpp"

std::string Router::makeKey(HttpMethod method, const std::string& path) {
    return methodToString(method) + " " + path;
}

void Router::addRoute(HttpMethod method, const std::string& path, HandlerFn handler) {
    m_routes[makeKey(method, path)] = std::move(handler);
}

HttpResponse Router::route(const HttpRequest& req) const {
    auto it = m_routes.find(makeKey(req.method, req.path));
    if (it != m_routes.end()) {
        return it->second(req);
    }

    HttpResponse res;
    res.statusCode = 404;
    res.statusText = "Not Found";
    res.headers["Content-Type"] = "text/plain";
    res.body = "404 Not Found";
    return res;
}
