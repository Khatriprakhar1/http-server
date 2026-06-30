#include "TcpServer.hpp"
#include "HttpParser.hpp"
#include <iostream>

int main() {
    try {
        TcpServer server(8080);

        server.addRoute(HttpMethod::GET, "/", [](const HttpRequest&) {
            HttpResponse res;
            res.body = "Welcome home!";
            res.headers["Content-Type"] = "text/plain";
            return res;
        });

        server.addRoute(HttpMethod::GET, "/health", [](const HttpRequest&) {
            HttpResponse res;
            res.body = "OK";
            res.headers["Content-Type"] = "text/plain";
            return res;
        });

        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}