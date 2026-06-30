#include "TcpServer.hpp"
#include "HttpParser.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>

TcpServer::TcpServer(int port, size_t numThreads)
    : m_port(port), m_serverFd(-1), m_running(false), m_pool(numThreads) {
    std::memset(&m_address, 0, sizeof(m_address));
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::setupSocket() {
    m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    m_address.sin_family      = AF_INET;
    m_address.sin_addr.s_addr = INADDR_ANY;
    m_address.sin_port        = htons(m_port);

    if (bind(m_serverFd, (struct sockaddr*)&m_address, sizeof(m_address)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(m_serverFd, 10) < 0)
        throw std::runtime_error("listen() failed");

    std::cout << "[TcpServer] Listening on port " << m_port << "\n";
}

void TcpServer::start() {
    setupSocket();
    m_running = true;

    while (m_running) {
        socklen_t addrLen = sizeof(m_address);
        int clientFd = accept(m_serverFd, (struct sockaddr*)&m_address, &addrLen);
        if (clientFd < 0) {
            if (m_running)
                std::cerr << "[TcpServer] accept() failed\n";
            break;
        }
        std::cout << "[TcpServer] Client connected: fd=" << clientFd << "\n";
        m_pool.enqueue([this, clientFd]() {
            onClientConnected(clientFd);
        });
    }
}

void TcpServer::stop() {
    m_running = false;
    if (m_serverFd >= 0) {
        close(m_serverFd);
        m_serverFd = -1;
    }
}

void TcpServer::addRoute(HttpMethod method, const std::string& path, HandlerFn handler) {
    m_router.addRoute(method, path, std::move(handler));
}

void TcpServer::onClientConnected(int clientFd) {
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    bool keepAlive = true;

    while (keepAlive) {
        char buffer[4096] = {0};
        ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);

        if (bytesRead <= 0) {
            break; // timeout, client closed, or error — exit loop and close
        }

        std::string rawRequest(buffer, bytesRead);
        HttpResponse res;
        HttpRequest req;

        try {
            req = HttpParser::parse(rawRequest);
            std::cout << "[Parsed] " << methodToString(req.method) << " " << req.path << "\n";
            res = m_router.route(req);
        } catch (const std::exception& e) {
            std::cerr << "[Parse error] " << e.what() << "\n";
            res.statusCode = 400;
            res.statusText = "Bad Request";
            res.body = "400 Bad Request";
            keepAlive = false;
        }

        // Decide whether to keep the connection open
        auto it = req.headers.find("Connection");
        if (it != req.headers.end() && it->second == "close") {
            keepAlive = false;
        } else if (req.version == "HTTP/1.0") {
            keepAlive = false; // HTTP/1.0 defaults to close unless explicitly keep-alive
        }

        res.headers["Connection"] = keepAlive ? "keep-alive" : "close";

        std::string responseStr = res.toString();
        write(clientFd, responseStr.c_str(), responseStr.size());

        if (!keepAlive) break;
    }

    close(clientFd);
}