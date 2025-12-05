// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

namespace Mcp::Internal {

/**
 * @brief HTTP/1.1 Request Parser
 *
 * Parses HTTP requests from raw TCP data and extracts:
 * - HTTP method (GET, POST, etc.)
 * - Request URI
 * - HTTP version
 * - Headers
 * - Body content
 *
 * Designed to work with the existing TCP MCP server to provide
 * HTTP compatibility without requiring the HttpServer module.
 */
class HttpParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief HTTP request structure
     */
    struct HttpRequest
    {
        QString method;                 ///< HTTP method (GET, POST, etc.)
        QString uri;                    ///< Request URI
        QString version;                ///< HTTP version (1.1, etc.)
        QMap<QString, QString> headers; ///< HTTP headers (case-insensitive keys)
        QByteArray body;                ///< Request body content
        bool isValid;                   ///< Whether the request is valid
        bool needMoreData;              ///< Whether the request has all data available
        QString errorMessage;           ///< Error message if parsing failed
    };

    explicit HttpParser(QObject *parent = nullptr);

    /**
     * @brief Parse HTTP request from raw TCP data
     * @param data Raw TCP data received from client
     * @return Parsed HTTP request structure
     */
    HttpRequest parseRequest(const QByteArray &data);

    /**
     * @brief Check if data looks like an HTTP request
     * @param data Raw data to check
     * @return true if data appears to be HTTP
     */
    static bool isHttpRequest(const QByteArray &data);

private:
    /**
     * @brief Parse the HTTP request line (method URI version)
     * @param requestLine First line of HTTP request
     * @param request Output request structure
     * @return true if parsing succeeded
     */
    bool parseRequestLine(const QString &requestLine, HttpRequest &request);

    /**
     * @brief Parse HTTP headers
     * @param headerLines List of header lines
     * @param request Output request structure
     */
    void parseHeaders(const QStringList &headerLines, HttpRequest &request);

    /**
     * @brief Normalize header name (lowercase for case-insensitive lookup)
     * @param headerName Header name to normalize
     * @return Normalized header name
     */
    static QString normalizeHeaderName(const QString &headerName);

    /**
     * @brief Extract body content from HTTP data
     * @param data Complete HTTP data
     * @param headerEnd Position where headers end
     * @param contentLength Content-Length header value
     * @return Body content
     */
    QByteArray extractBody(const QByteArray &data, int headerEnd, int contentLength);

    /**
     * @brief Split HTTP data into lines
     * @param data HTTP data to split
     * @return List of lines
     */
    QStringList splitHttpLines(const QByteArray &data);
};

} // namespace Mcp::Internal
