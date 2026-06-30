#include "TcpServer.hpp"
#include "HttpParser.hpp"
#include <iostream>
#include <cstdlib>

int main() {
    try {
        int port = 8080;
        if (const char* envPort = std::getenv("PORT")) {
            port = std::atoi(envPort);
        }

        TcpServer server(port);

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

        std::cout << "Starting server on port " << port << "\n";
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}