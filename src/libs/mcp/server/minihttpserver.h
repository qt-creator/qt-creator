// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifndef MCP_SERVER_HAS_QT_HTTP_SERVER

// Minimal HTTP/1.1 server replacement for QHttpServer.
// Used when QtHttpServer module is not available (Qt < 6.11).
// QtHttpServer before 6.11 does not support some features needed
// for sse support.
//
// Provides drop-in replacements for:
//   QHttpServerRequest   → MiniHttp::HttpRequest
//   QHttpServerResponder → MiniHttp::HttpResponder
//   QHttpServerResponse  → MiniHttp::HttpResponse
//   QHttpServer          → MiniHttp::HttpServer

#include <QAbstractSocket>
#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>

namespace MiniHttp {

enum class StatusCode : int {
    Ok = 200,
    Accepted = 202,
    NoContent = 204,
    BadRequest = 400,
    NotFound = 404,
};

inline QByteArray statusText(StatusCode code)
{
    switch (code) {
    case StatusCode::Ok:        return "OK";
    case StatusCode::Accepted:
        return "Accepted";
    case StatusCode::NoContent: return "No Content";
    case StatusCode::BadRequest: return "Bad Request";
    case StatusCode::NotFound:  return "Not Found";
    }
    return "Unknown";
}

class HttpHeaders
{
public:
    void append(const QByteArray &name, const QByteArray &value) { set(name, value); }

    void set(const QByteArray &name, const QByteArray &value)
    {
        m_map[name.toLower()] = value.trimmed();
    }

    bool contains(const QByteArray &name) const
    {
        return m_map.contains(name.toLower());
    }

    QByteArray value(const QByteArray &name) const
    {
        return m_map.value(name.toLower());
    }

    const QMap<QByteArray, QByteArray> &map() const { return m_map; }

private:
    QMap<QByteArray, QByteArray> m_map;
};

inline QDebug operator<<(QDebug dbg, const HttpHeaders &headers)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    for (auto it = headers.map().constBegin(); it != headers.map().constEnd(); ++it)
        dbg << it.key() << ": " << it.value() << "\n";
    return dbg;
}

class HttpRequest
{
public:
    enum class Method { Get, Post, Options, Delete, Other };

    HttpRequest() = default;

    // Parse a complete HTTP/1.1 request from raw bytes.
    // Returns nullopt when more data is needed.
    // On success, *consumed is set to the number of bytes consumed.
    static std::optional<HttpRequest> parse(const QByteArray &data, int &consumed)
    {
        // Locate end-of-headers sentinel
        const int headerEnd = data.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return std::nullopt;

        HttpRequest req;
        const QList<QByteArray> lines = data.left(headerEnd).split('\n');
        if (lines.isEmpty())
            return std::nullopt;

        // Request line: METHOD SP Request-URI SP HTTP-Version
        const QList<QByteArray> requestLine = lines[0].trimmed().split(' ');
        if (requestLine.size() < 2)
            return std::nullopt;

        const QByteArray method = requestLine[0].trimmed().toUpper();
        if (method == "GET")
            req.m_method = Method::Get;
        else if (method == "POST")
            req.m_method = Method::Post;
        else if (method == "OPTIONS")
            req.m_method = Method::Options;
        else if (method == "DELETE")
            req.m_method = Method::Delete;
        else
            req.m_method = Method::Other;

        req.m_url = QUrl(QString::fromUtf8(requestLine[1].trimmed()));

        // Header fields
        for (int i = 1; i < lines.size(); ++i) {
            const QByteArray line = lines[i].trimmed();
            if (line.isEmpty())
                continue;
            const int colon = line.indexOf(':');
            if (colon < 0)
                continue;
            req.m_headers.set(line.left(colon).trimmed(), line.mid(colon + 1).trimmed());
        }

        // Message body (bounded by Content-Length)
        const int bodyStart = headerEnd + 4; // skip "\r\n\r\n"
        const int contentLength = req.m_headers.value("content-length").toInt();
        if (data.size() - bodyStart < contentLength)
            return std::nullopt; // need more data

        req.m_body = data.mid(bodyStart, contentLength);
        consumed = bodyStart + contentLength;
        return req;
    }

    QUrl url() const { return m_url; }
    QUrlQuery query() const { return QUrlQuery(m_url); }
    Method method() const { return m_method; }
    const HttpHeaders &headers() const { return m_headers; }
    QByteArray body() const { return m_body; }

private:
    QUrl m_url;
    Method m_method = Method::Other;
    HttpHeaders m_headers;
    QByteArray m_body;
};

inline QDebug operator<<(QDebug dbg, HttpRequest::Method method)
{
    switch (method) {
    case HttpRequest::Method::Get:   return dbg << "GET";
    case HttpRequest::Method::Post:  return dbg << "POST";
    case HttpRequest::Method::Options: return dbg << "OPTIONS";
    case HttpRequest::Method::Delete: return dbg << "DELETE";
    case HttpRequest::Method::Other: return dbg << "OTHER";
    }
    return dbg << "UNKNOWN";
}

// Move-only responder that writes raw HTTP responses over a QTcpSocket.
// It holds a QPointer so it is safe to outlive the socket (SSE case where the
// client disconnects before the SseStream destructor runs).

class HttpResponder
{
public:
    using StatusCode = MiniHttp::StatusCode;

    explicit HttpResponder(QTcpSocket *socket)
        : m_socket(socket)
    {}

    HttpResponder(HttpResponder &&other) noexcept
        : m_socket(std::exchange(other.m_socket, nullptr))
        , m_chunked(other.m_chunked)
    {}

    HttpResponder &operator=(HttpResponder &&other) noexcept
    {
        m_socket = std::exchange(other.m_socket, nullptr);
        m_chunked = other.m_chunked;
        return *this;
    }

    HttpResponder(const HttpResponder &) = delete;
    HttpResponder &operator=(const HttpResponder &) = delete;

    ~HttpResponder() = default;

    void write(const QByteArray &data, const char *contentType, StatusCode status)
    {
        if (!m_socket)
            return;
        QByteArray response;
        response.reserve(128 + data.size());
        appendStatusLine(response, status);
        response += "Content-Type: ";
        response += contentType;
        response += "\r\nContent-Length: ";
        response += QByteArray::number(data.size());
        response += "\r\nConnection: close\r\n\r\n";
        response += data;
        writeToSocket(response);
        closeSocket();
    }

    void write(const QByteArray &data, const HttpHeaders &headers, StatusCode status)
    {
        if (!m_socket)
            return;
        QByteArray response;
        response.reserve(128 + data.size());
        appendStatusLine(response, status);
        for (auto it = headers.map().constBegin(); it != headers.map().constEnd(); ++it) {
            response += it.key();
            response += ": ";
            response += it.value();
            response += "\r\n";
        }
        response += "Content-Length: ";
        response += QByteArray::number(data.size());
        response += "\r\nConnection: close\r\n\r\n";
        response += data;
        writeToSocket(response);
        closeSocket();
    }

    void write(const HttpHeaders &headers, StatusCode status)
    {
        if (!m_socket)
            return;
        QByteArray response;
        appendStatusLine(response, status);
        for (auto it = headers.map().constBegin(); it != headers.map().constEnd(); ++it) {
            response += it.key();
            response += ": ";
            response += it.value();
            response += "\r\n";
        }
        response += "Content-Length: 0\r\nConnection: close\r\n\r\n";
        writeToSocket(response);
        closeSocket();
    }

    void write(StatusCode status)
    {
        if (!m_socket)
            return;
        QByteArray response;
        appendStatusLine(response, status);
        response += "Content-Length: 0\r\nConnection: close\r\n\r\n";
        writeToSocket(response);
        closeSocket();
    }

    void writeBeginChunked(const HttpHeaders &headers, StatusCode status)
    {
        if (!m_socket)
            return;
        QByteArray response;
        appendStatusLine(response, status);
        for (auto it = headers.map().constBegin(); it != headers.map().constEnd(); ++it) {
            response += it.key();
            response += ": ";
            response += it.value();
            response += "\r\n";
        }
        response += "\r\nTransfer-Encoding: chunked\r\n"
                    "Cache-Control: no-cache\r\n"
                    "Connection: keep-alive\r\n"
                    "\r\n";
        writeToSocket(response);
        m_chunked = true;
    }

    void writeChunk(const QByteArray &data)
    {
        if (!m_socket || isResponseCanceled())
            return;
        QByteArray chunk;
        chunk += QByteArray::number(data.size(), 16);
        chunk += "\r\n";
        chunk += data;
        chunk += "\r\n";
        writeToSocket(chunk);
    }

    void writeEndChunked(const QByteArray &data)
    {
        if (!m_socket)
            return;
        if (m_chunked)
            writeChunk(data);
        closeSocket();
    }

    bool isResponseCanceled() const
    {
        return !m_socket
               || m_socket->state() == QAbstractSocket::UnconnectedState
               || m_socket->state() == QAbstractSocket::ClosingState;
    }

    QTcpSocket *socket() const { return m_socket.data(); }

private:
    void appendStatusLine(QByteArray &out, StatusCode status) const
    {
        out += "HTTP/1.1 ";
        out += QByteArray::number(static_cast<int>(status));
        out += ' ';
        out += statusText(status);
        out += "\r\n";
    }

    void writeToSocket(const QByteArray &data)
    {
        if (!m_socket)
            return;
        m_socket->write(data);
        m_socket->flush();
    }

    void closeSocket()
    {
        if (!m_socket)
            return;
        m_socket->disconnectFromHost();
        m_socket = nullptr; // release the QPointer
    }

    QPointer<QTcpSocket> m_socket;
    bool m_chunked = false;
};

// Immutable value type returned by route handlers that do not need a Responder.

class HttpResponse
{
public:
    using StatusCode = MiniHttp::StatusCode;

    HttpResponse(const QByteArray &data, const char *contentType, StatusCode status)
        : m_data(data)
        , m_status(status)
    {
        m_headers.append("Content-Type:", contentType);
    }

    // Convenience for HTML strings
    HttpResponse(const char *html, StatusCode status)
        : m_data(html)
        , m_status(status)
    {
        m_headers.append("Content-Type:", "text/html");
    }

    HttpResponse(StatusCode status)
        : m_status(status)
    {
        m_headers.append("Content-Type:", "text/plain");
    }

    void writeTo(HttpResponder &responder) const
    {
        responder.write(m_data, m_headers, m_status);
    }

    void setHeaders(const HttpHeaders &headers) { m_headers = headers; }

private:
    QByteArray m_data;
    StatusCode m_status;
    HttpHeaders m_headers;
};

// Minimal routing HTTP/1.1 server built on QTcpServer.

class HttpServer : public QObject
{
    Q_OBJECT

public:
    using StatusCode = MiniHttp::StatusCode;

    explicit HttpServer(QObject *parent = nullptr)
        : QObject(parent)
    {}

    ~HttpServer() override = default;

    // Attach to an existing QTcpServer (does not take ownership).
    bool bind(QTcpServer *server)
    {
        if (!server || !server->isListening())
            return false;
        m_servers.append(server);
        connect(server, &QTcpServer::newConnection, this, [this, server]() {
            onNewConnection(server);
        });
        connect(server, &QTcpServer::destroyed, this, [this, server]() {
            m_servers.removeAll(server);
        });
        return true;
    }

    QList<QTcpServer *> servers() const { return m_servers; }

    // Handler receives (request, responder) and must write the response itself.
    void route(const QByteArray &path,
               HttpRequest::Method method,
               std::function<void(const HttpRequest &, HttpResponder &)> handler)
    {
        m_routes.append({path, method, std::move(handler)});
    }

    // Handler takes no arguments and returns an HttpResponse value.
    void route(const QByteArray &path,
               HttpRequest::Method method,
               std::function<HttpResponse()> handler)
    {
        route(path, method, [h = std::move(handler)](const HttpRequest &, HttpResponder &resp) {
            h().writeTo(resp);
        });
    }

    // Catch-all template allowing lambdas to be passed directly without an
    // explicit std::function cast.  Dispatches to the most appropriate overload
    // based on the callable's signature.
    template<typename Handler>
    auto route(const QByteArray &path, HttpRequest::Method method, Handler &&handler)
        -> std::enable_if_t<
            !std::is_same_v<std::decay_t<Handler>,
                            std::function<void(const HttpRequest &, HttpResponder &)>>
            && !std::is_same_v<std::decay_t<Handler>,
                               std::function<HttpResponse()>>>
    {
        if constexpr (std::is_invocable_v<Handler, const HttpRequest &, HttpResponder &>) {
            route(path,
                  method,
                  std::function<void(const HttpRequest &, HttpResponder &)>(
                      std::forward<Handler>(handler)));
        } else if constexpr (std::is_invocable_r_v<HttpResponse, Handler>) {
            route(path, method, std::function<HttpResponse()>(std::forward<Handler>(handler)));
        }
    }

    // Missing-route handler.  The QObject* parent argument is accepted for API
    // compatibility but ignored.
    void setMissingHandler(QObject * /*parent*/,
                           std::function<void(const HttpRequest &, HttpResponder &)> handler)
    {
        m_missingHandler = std::move(handler);
    }

private:
    struct Route
    {
        QByteArray path;
        HttpRequest::Method method;
        std::function<void(const HttpRequest &, HttpResponder &)> handler;
    };

    struct Connection
    {
        QPointer<QTcpSocket> socket;
        QByteArray buffer;
    };

    void onNewConnection(QTcpServer *server)
    {
        while (server->hasPendingConnections()) {
            QTcpSocket *socket = server->nextPendingConnection();
            if (!socket)
                continue;

            // Each socket gets its own heap-allocated read buffer.
            auto *buf = new QByteArray();

            connect(socket, &QTcpSocket::readyRead, this, [this, socket, buf]() {
                buf->append(socket->readAll());
                int consumed = 0;
                auto req = HttpRequest::parse(*buf, consumed);
                if (!req)
                    return; // need more data
                *buf = buf->mid(consumed);
                dispatchRequest(*req, socket);
            });

            connect(socket, &QTcpSocket::disconnected, this, [socket, buf]() {
                delete buf;
                socket->deleteLater();
            });

            // Guard against premature socket closure before readyRead fires
            connect(socket, &QTcpSocket::errorOccurred, this, [socket, buf](auto) {
                Q_UNUSED(buf);
                socket->deleteLater();
            });
        }
    }

    void dispatchRequest(const HttpRequest &req, QTcpSocket *socket)
    {
        const QByteArray path = req.url().path().toUtf8();

        for (const Route &route : m_routes) {
            if (route.path == path && route.method == req.method()) {
                HttpResponder responder(socket);
                route.handler(req, responder);
                return;
            }
        }

        // Fall through to missing handler
        HttpResponder responder(socket);
        if (m_missingHandler)
            m_missingHandler(req, responder);
        else
            responder.write(StatusCode::NotFound);
    }

    QList<QTcpServer *> m_servers;
    QList<Route> m_routes;
    std::function<void(const HttpRequest &, HttpResponder &)> m_missingHandler;
};

} // namespace MiniHttp

#endif
