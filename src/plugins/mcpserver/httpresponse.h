// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>

namespace Mcp::Internal {

/**
 * @brief HTTP Response Builder
 *
 * Builds HTTP/1.1 responses with proper headers and formatting.
 * Designed to work with the existing TCP MCP server to provide
 * HTTP compatibility without requiring the HttpServer module.
 */
class HttpResponse : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief HTTP status codes
     */
    enum StatusCode {
        OK = 200,
        CREATED = 201,
        NO_CONTENT = 204,
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500,
        NOT_IMPLEMENTED = 501,
        BAD_GATEWAY = 502,
        SERVICE_UNAVAILABLE = 503
    };

    /**
     * @brief HTTP response structure
     */
    struct ResponseData
    {
        StatusCode statusCode;
        QString statusMessage;
        QMap<QString, QString> headers;
        QByteArray body;
        QString version;
    };

    explicit HttpResponse(QObject *parent = nullptr);

    /**
     * @brief Create a successful response with JSON body
     * @param jsonBody JSON content to send
     * @param statusCode HTTP status code (default: 200 OK)
     * @return Formatted HTTP response
     */
    static QByteArray createJsonResponse(const QByteArray &jsonBody, StatusCode statusCode = OK);

    /**
     * @brief Create a simple text response
     * @param textBody Text content to send
     * @param statusCode HTTP status code (default: 200 OK)
     * @return Formatted HTTP response
     */
    static QByteArray createTextResponse(const QString &textBody, StatusCode statusCode = OK);

    /**
     * @brief Create an error response
     * @param statusCode HTTP status code
     * @param errorMessage Error message to include
     * @return Formatted HTTP response
     */
    static QByteArray createErrorResponse(StatusCode statusCode, const QString &errorMessage);

    /**
     * @brief Create a CORS-enabled response
     * @param jsonBody JSON content to send
     * @param statusCode HTTP status code (default: 200 OK)
     * @return Formatted HTTP response with CORS headers
     */
    static QByteArray createCorsResponse(const QByteArray &jsonBody, StatusCode statusCode = OK);

    /**
     * @brief Build HTTP response from response data
     * @param response Response data structure
     * @return Formatted HTTP response
     */
    static QByteArray buildResponse(const ResponseData &response);

private:
    /**
     * @brief Get status message for status code
     * @param statusCode HTTP status code
     * @return Status message string
     */
    static QString getStatusMessage(StatusCode statusCode);

    /**
     * @brief Format HTTP response line
     * @param version HTTP version
     * @param statusCode HTTP status code
     * @param statusMessage Status message
     * @return Formatted status line
     */
    static QString formatStatusLine(
        const QString &version, StatusCode statusCode, const QString &statusMessage);

    /**
     * @brief Format HTTP headers
     * @param headers Header map
     * @return Formatted header string
     */
    static QString formatHeaders(const QMap<QString, QString> &headers);
};

} // namespace Mcp::Internal
