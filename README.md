# HTTP/1.1 Server from Scratch

A multi-threaded HTTP/1.1 server built in C++ using raw POSIX sockets — no networking libraries, no HTTP frameworks. Every layer of the stack, from the TCP accept loop to HTTP parsing to request routing, is implemented by hand.

**Live demo:** https://http-server-9d55.onrender.com/
**Health check:** https://http-server-9d55.onrender.com/health

```bash
curl https://http-server-9d55.onrender.com/
curl https://http-server-9d55.onrender.com/health
```

## Why this project

Most backend projects reach for a framework (`cpp-httplib`, `Boost.Beast`, Express, Flask) and never see what happens underneath. This project goes the other direction — it's built directly on the POSIX socket API, the same kernel interface every one of those frameworks is ultimately built on top of. The result touches three areas directly:

- **Computer networks** — the TCP three-way handshake, raw socket lifecycle, HTTP/1.1 wire format
- **Operating systems** — concurrency with a hand-written thread pool, mutexes, condition variables, race conditions
- **Backend engineering** — request parsing, routing, connection lifecycle, keep-alive

## Architecture

The server is built in five stages, each adding one layer of real-world server behavior:

| Stage | Component | What it adds |
|---|---|---|
| 1 | `TcpServer` | Raw socket accept loop — `socket()`, `bind()`, `listen()`, `accept()` |
| 2 | `HttpParser` | Parses raw request bytes into method, path, headers, body |
| 3 | `ThreadPool` | Concurrent request handling via a fixed worker pool and task queue |
| 4 | `Router` | Maps method + path to handler functions, with keep-alive and idle timeouts |
| 5 | Docker | Multi-stage build, containerized deployment |

### Request flow

```
client → accept() → ThreadPool.enqueue() → HttpParser.parse() → Router.route() → HttpResponse → write()
```

`TcpServer::start()` runs a single-threaded `accept()` loop. Each accepted connection is handed off to the thread pool immediately rather than processed inline, so the main loop is never blocked waiting on a slow client — it just keeps accepting.

## Key design decisions

**Virtual hook separates TCP from HTTP.** `TcpServer::onClientConnected()` is `virtual`. The base class only knows about file descriptors and socket lifecycle; it has no concept of HTTP. `HttpServer` (or in this implementation, the parsing/routing logic layered on top) overrides this hook to do the actual request handling. This keeps networking concerns and protocol concerns in separate, independently testable classes.

**The thread pool releases its lock before running tasks.** Inside `ThreadPool::workerLoop()`, the queue mutex protects only the push/pop operations on the task queue — it's released before the task itself executes:

```cpp
void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
            if (m_stop && m_tasks.empty()) return;
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        task(); // lock is released here — this is the point
    }
}
```

If the lock were held during `task()`, only one client could be processed at a time regardless of pool size, defeating the entire purpose of having multiple worker threads.

**Keep-alive with a read timeout.** HTTP/1.1 connections default to keep-alive, so the server loops on `read()` for the same client socket rather than closing after every response. To prevent a silent or malicious client from holding a thread pool slot forever, the socket is configured with `SO_RCVTIMEO` — an idle connection is dropped after 5 seconds of inactivity rather than blocking indefinitely.

**Multi-stage Docker build.** The build stage installs the full C++ toolchain (`g++`, `cmake`, `make`) and compiles the binary. The final image starts from a clean base and copies in only the compiled executable via `COPY --from=build`. The shipped image carries no compiler, no build tools, and no source code — just the binary and its minimal runtime dependency (`libstdc++6`).

## Project structure

```
http-server/
├── include/          # Header files (.hpp)
│   ├── TcpServer.hpp
│   ├── HttpParser.hpp
│   ├── HttpTypes.hpp
│   ├── ThreadPool.hpp
│   └── Router.hpp
├── src/               # Implementation files (.cpp)
│   ├── TcpServer.cpp
│   ├── HttpParser.cpp
│   ├── ThreadPool.cpp
│   └── Router.cpp
├── main.cpp           # Entry point, route registration
├── CMakeLists.txt
├── Dockerfile          # Multi-stage build
└── .dockerignore
```

## Building and running locally

Requires `g++`, `cmake`, and `make` (Linux or WSL on Windows).

```bash
mkdir build && cd build
cmake ..
make
./http_server
```

Server listens on port `8080` by default, or the value of the `PORT` environment variable if set (used for cloud deployment).

Test it:

```bash
curl http://localhost:8080/
curl http://localhost:8080/health
```

## Running with Docker

```bash
docker build -t http-server .
docker run -p 8080:8080 http-server
```

## API

| Method | Path | Response |
|---|---|---|
| `GET` | `/` | `Welcome home!` |
| `GET` | `/health` | `OK` |
| any | unregistered path | `404 Not Found` |

## Verifying concurrency and keep-alive

Two requests fired at once land on separate threads from the pool:

```bash
curl http://localhost:8080/a & curl http://localhost:8080/b &
```

Server log shows both `accept()` calls completing before either request finishes parsing — proof the second connection wasn't blocked behind the first.

Keep-alive connection reuse, observable with curl's verbose flag:

```bash
curl http://localhost:8080/ http://localhost:8080/health -v
```

Look for `Re-using existing connection with host localhost` on the second request — confirms one TCP connection carried two full request/response cycles, with no second handshake.

## What's not yet implemented

- Path parameters (`/users/:id`) — current router is exact-match only via a `std::map`. A trie or ordered pattern list would be the natural next step.
- IPv6 — the socket is bound with `AF_INET` only.
- HTTP pipelining and chunked transfer encoding.
- TLS termination — handled by the deployment platform (Render) rather than the server itself.

## Deployment

Deployed on Render via the included `Dockerfile`. Render builds the image directly from the GitHub repository on every push to `main` and injects a `PORT` environment variable at runtime, which `main.cpp` reads via `std::getenv("PORT")`.
