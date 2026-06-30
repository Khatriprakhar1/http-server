#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ThreadPool.hpp"
#include "Router.hpp"

class TcpServer {
public:
    explicit TcpServer(int port, size_t numThreads = 4);
    virtual ~TcpServer();

    void start();
    void stop();
    void addRoute(HttpMethod method, const std::string& path, HandlerFn handler);

protected:
    virtual void onClientConnected(int clientFd);

private:
    int  m_port;
    int  m_serverFd;
    bool m_running;
    struct sockaddr_in m_address;
    ThreadPool m_pool;
    Router m_router;

    void setupSocket();
};