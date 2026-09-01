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
//
// Only the subset of HTTP/1.1 documented here is understood. Anything else is
// rejected with a status code rather than reinterpreted: no chunked transfer
// coding, no obs-fold, no HTTP/2. Requests exceeding the limits below are
// rejected as well, so a peer that can reach the port cannot make the server
// allocate or compute without bound.

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

#include <cstring>
#include <functional>
#include <memory>
#include <type_traits>

namespace MiniHttp {

// Ceilings on anything a peer controls. The values follow QtHttpServer's
// documented defaults, so both configurations refuse the same requests.
constexpr qsizetype MaxHeaderBlockSize = 64 * 1024;
constexpr qsizetype MaxBodySize = 32 * 1024 * 1024;
constexpr qsizetype MaxWriteBufferSize = 32 * 1024 * 1024;
constexpr int MaxHeaderCount = 128;
constexpr int MaxConnections = 64;

enum class StatusCode : int {
    Ok = 200,
    Accepted = 202,
    NoContent = 204,
    BadRequest = 400,
    NotFound = 404,
    PayloadTooLarge = 413,
    RequestHeaderFieldsTooLarge = 431,
    NotImplemented = 501,
    ServiceUnavailable = 503,
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
    case StatusCode::PayloadTooLarge: return "Content Too Large";
    case StatusCode::RequestHeaderFieldsTooLarge: return "Request Header Fields Too Large";
    case StatusCode::NotImplemented: return "Not Implemented";
    case StatusCode::ServiceUnavailable: return "Service Unavailable";
    }
    return "Unknown";
}

class HttpHeaders
{
public:
    // Repeated fields are combined into a comma-separated list, as HTTP
    // defines them. A field that must be single-valued therefore fails to
    // parse rather than resolving to one of the values by insertion order.
    void append(const QByteArray &name, const QByteArray &value)
    {
        const QByteArray key = name.toLower();
        auto it = m_map.find(key);
        if (it == m_map.end())
            m_map.insert(key, value.trimmed());
        else
            *it += ", " + value.trimmed();
    }

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

    enum class ParseResult {
        Incomplete, // More data needed; nothing is wrong with what arrived so far.
        Complete,   // Header block parsed; the body is described by bodyStart/contentLength.
        Rejected,   // Malformed or oversized; send status and close.
    };

    HttpRequest() = default;

    // Parse the header block of an HTTP/1.1 request. The body is left to the
    // caller, which knows how much has arrived; nothing here depends on a byte
    // count derived from the peer's own Content-Length.
    //
    // searchStart carries the offset up to which the buffer is known to hold no
    // end of header block, so a block that arrives over many reads is scanned
    // once rather than once per read. It is an in/out parameter; pass a zero for
    // the first read of a request and hand the same variable back afterwards.
    static ParseResult parseHeaders(const QByteArray &data,
                                    HttpRequest *request,
                                    qsizetype *searchStart,
                                    qsizetype *bodyStart,
                                    qsizetype *contentLength,
                                    StatusCode *status)
    {
        *status = StatusCode::BadRequest;

        qsizetype start = 0;
        const qsizetype headerEnd = findHeaderEnd(data, *searchStart, &start);
        if (headerEnd < 0) {
            // A LF in the last two bytes cannot be decided yet: the CR or LF
            // that would terminate the block may still be on its way. Everything
            // before that has been ruled out for good.
            *searchStart = qMax(qsizetype(0), data.size() - 2);
            if (data.size() > MaxHeaderBlockSize) {
                *status = StatusCode::RequestHeaderFieldsTooLarge;
                return ParseResult::Rejected;
            }
            return ParseResult::Incomplete;
        }
        *searchStart = 0;
        if (headerEnd > MaxHeaderBlockSize) {
            *status = StatusCode::RequestHeaderFieldsTooLarge;
            return ParseResult::Rejected;
        }

        const QList<QByteArray> lines = data.left(headerEnd).split('\n');

        // Request line: METHOD SP Request-URI SP HTTP-Version
        const QList<QByteArray> requestLine = lines.constFirst().simplified().split(' ');
        if (requestLine.size() != 3 || !requestLine.at(2).startsWith("HTTP/"))
            return ParseResult::Rejected;

        const QByteArray method = requestLine.at(0).toUpper();
        if (method == "GET")
            request->m_method = Method::Get;
        else if (method == "POST")
            request->m_method = Method::Post;
        else if (method == "OPTIONS")
            request->m_method = Method::Options;
        else if (method == "DELETE")
            request->m_method = Method::Delete;
        else
            request->m_method = Method::Other;

        request->m_url = QUrl(QString::fromUtf8(requestLine.at(1)));
        if (!request->m_url.isValid())
            return ParseResult::Rejected;

        int headerCount = 0;
        for (qsizetype i = 1; i < lines.size(); ++i) {
            QByteArray line = lines.at(i);
            if (line.endsWith('\r'))
                line.chop(1);
            if (line.isEmpty())
                continue;
            if (line.startsWith(' ') || line.startsWith('\t')) // obs-fold
                return ParseResult::Rejected;
            const qsizetype colon = line.indexOf(':');
            if (colon <= 0)
                return ParseResult::Rejected;
            const QByteArray name = line.left(colon);
            if (!isFieldName(name))
                return ParseResult::Rejected;
            if (++headerCount > MaxHeaderCount) {
                *status = StatusCode::RequestHeaderFieldsTooLarge;
                return ParseResult::Rejected;
            }
            request->m_headers.append(name, line.mid(colon + 1));
        }

        if (request->m_headers.contains("transfer-encoding")) {
            *status = StatusCode::NotImplemented;
            return ParseResult::Rejected;
        }

        qsizetype length = 0;
        if (request->m_headers.contains("content-length")) {
            bool ok = false;
            length = request->m_headers.value("content-length").toLongLong(&ok);
            if (!ok || length < 0)
                return ParseResult::Rejected;
            if (length > MaxBodySize) {
                *status = StatusCode::PayloadTooLarge;
                return ParseResult::Rejected;
            }
        }

        *bodyStart = start;
        *contentLength = length;
        return ParseResult::Complete;
    }

    void setBody(const QByteArray &body) { m_body = body; }

    QUrl url() const { return m_url; }
    QUrlQuery query() const { return QUrlQuery(m_url); }
    Method method() const { return m_method; }
    const HttpHeaders &headers() const { return m_headers; }
    QByteArray body() const { return m_body; }

private:
    // End of the header block, or -1 while it has not arrived yet. CRLFCRLF,
    // CRLFLF, LFCRLF and LFLF all terminate it, as they do for QHttpServer, so
    // a request cannot be terminated at a boundary its sender did not intend.
    static qsizetype findHeaderEnd(const QByteArray &data, qsizetype from, qsizetype *bodyStart)
    {
        for (qsizetype i = data.indexOf('\n', from); i >= 0; i = data.indexOf('\n', i + 1)) {
            qsizetype next = i + 1;
            if (next < data.size() && data.at(next) == '\r')
                ++next;
            if (next < data.size() && data.at(next) == '\n') {
                *bodyStart = next + 1;
                return i > 0 && data.at(i - 1) == '\r' ? i - 1 : i;
            }
        }
        return -1;
    }

    static bool isFieldName(const QByteArray &name)
    {
        if (name.isEmpty())
            return false;
        for (const char c : name) {
            if (c <= ' ' || c == 0x7f || std::strchr("()<>@,;:\\\"/[]?={}", c))
                return false;
        }
        return true;
    }

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
        HttpHeaders headers;
        headers.set("content-type", contentType);
        write(data, headers, status);
    }

    void write(const QByteArray &data, const HttpHeaders &headers, StatusCode status)
    {
        if (!m_socket)
            return;
        HttpHeaders out = headers;
        out.set("content-length", QByteArray::number(data.size()));
        out.set("connection", "close");
        QByteArray response;
        response.reserve(128 + data.size());
        appendStatusLine(response, status);
        appendHeaders(response, out);
        response += "\r\n";
        response += data;
        writeToSocket(response);
        closeSocket();
    }

    void write(const HttpHeaders &headers, StatusCode status)
    {
        write({}, headers, status);
    }

    void write(StatusCode status)
    {
        write({}, HttpHeaders{}, status);
    }

    void writeBeginChunked(const HttpHeaders &headers, StatusCode status)
    {
        if (!m_socket)
            return;
        HttpHeaders out = headers;
        out.set("transfer-encoding", "chunked");
        out.set("cache-control", "no-cache");
        out.set("connection", "keep-alive");
        QByteArray response;
        appendStatusLine(response, status);
        appendHeaders(response, out);
        response += "\r\n";
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

    // Strips CR and LF, so that a value reaching here cannot terminate the
    // header block early or inject a field of its own.
    void appendHeaders(QByteArray &out, const HttpHeaders &headers) const
    {
        for (auto it = headers.map().constBegin(); it != headers.map().constEnd(); ++it) {
            out += sanitized(it.key());
            out += ": ";
            out += sanitized(it.value());
            out += "\r\n";
        }
    }

    static QByteArray sanitized(const QByteArray &field)
    {
        QByteArray result = field;
        result.removeIf([](char c) { return c == '\r' || c == '\n'; });
        return result;
    }

    void writeToSocket(const QByteArray &data)
    {
        if (!m_socket)
            return;
        if (m_socket->bytesToWrite() > MaxWriteBufferSize) {
            m_socket->abort();
            m_socket = nullptr;
            return;
        }
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
        m_headers.set("Content-Type", contentType);
    }

    // Convenience for HTML strings
    HttpResponse(const char *html, StatusCode status)
        : m_data(html)
        , m_status(status)
    {
        m_headers.set("Content-Type", "text/html");
    }

    HttpResponse(StatusCode status)
        : m_status(status)
    {
        m_headers.set("Content-Type", "text/plain");
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

    // Per-connection parse state. Once the header block is parsed it is not
    // parsed again while the body arrives, and the search for the end of the
    // header block resumes where it stopped instead of restarting at byte 0.
    struct Connection
    {
        QByteArray buffer;
        HttpRequest request;
        qsizetype searchStart = 0;
        qsizetype bodyStart = -1;
        qsizetype contentLength = 0;
    };

    void onNewConnection(QTcpServer *server)
    {
        while (server->hasPendingConnections()) {
            QTcpSocket *socket = server->nextPendingConnection();
            if (!socket)
                continue;

            if (m_connectionCount >= MaxConnections) {
                socket->abort();
                socket->deleteLater();
                continue;
            }
            ++m_connectionCount;
            connect(socket, &QObject::destroyed, this, [this]() { --m_connectionCount; });

            // The state is shared with the handlers instead of owned by one of
            // them, so no handler can leave the others with a dangling pointer.
            auto state = std::make_shared<Connection>();

            connect(socket, &QTcpSocket::readyRead, this, [this, socket, state]() {
                state->buffer += socket->readAll();
                processBuffer(socket, *state);
            });

            connect(socket, &QTcpSocket::disconnected, this, [socket]() {
                socket->deleteLater();
            });

            connect(socket, &QTcpSocket::errorOccurred, this, [socket](auto) {
                socket->deleteLater();
            });
        }
    }

    void processBuffer(QTcpSocket *socket, Connection &state)
    {
        QPointer<QTcpSocket> guard(socket);
        while (guard && guard->state() == QAbstractSocket::ConnectedState) {
            if (state.bodyStart < 0) {
                state.request = {};
                StatusCode status = StatusCode::BadRequest;
                const HttpRequest::ParseResult result
                    = HttpRequest::parseHeaders(state.buffer,
                                                &state.request,
                                                &state.searchStart,
                                                &state.bodyStart,
                                                &state.contentLength,
                                                &status);
                if (result == HttpRequest::ParseResult::Rejected) {
                    HttpResponder responder(socket);
                    responder.write(status);
                    return;
                }
                if (result == HttpRequest::ParseResult::Incomplete)
                    return;
            }

            if (state.buffer.size() - state.bodyStart < state.contentLength)
                return; // need more data

            state.request.setBody(state.buffer.mid(state.bodyStart, state.contentLength));
            state.buffer.remove(0, state.bodyStart + state.contentLength);
            state.searchStart = 0;
            state.bodyStart = -1;
            state.contentLength = 0;

            dispatchRequest(state.request, socket);

            // A handler may run a nested event loop - an MCP tool that opens a
            // modal dialog does. QAbstractSocket does not emit readyRead while
            // one is already being handled, so whatever arrived meanwhile is
            // still in the socket, and no further read is coming for it if the
            // peer has said all it had to say.
            if (guard)
                state.buffer += guard->readAll();
        }
    }

    void dispatchRequest(const HttpRequest &req, QTcpSocket *socket)
    {
        const QByteArray path = req.url().path().toUtf8();

        for (const Route &route : std::as_const(m_routes)) {
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
    int m_connectionCount = 0;
};

} // namespace MiniHttp

#endif
