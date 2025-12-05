// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "httpresponse.h"

#include <QDebug>
#include <QTextStream>

namespace Mcp {
namespace Internal {

HttpResponse::HttpResponse(QObject *parent)
    : QObject(parent)
{}

QByteArray HttpResponse::createJsonResponse(const QByteArray &jsonBody, StatusCode statusCode)
{
    ResponseData response;
    response.statusCode = statusCode;
    response.statusMessage = getStatusMessage(statusCode);
    response.version = "1.1";
    response.body = jsonBody;

    // Set standard headers for JSON response
    response.headers["Content-Type"] = "application/json; charset=utf-8";
    response.headers["Content-Length"] = QString::number(jsonBody.size());
    response.headers["Server"] = "Qt MCP Plugin HTTP Server";
    response.headers["Connection"] = "close";

    return buildResponse(response);
}

QByteArray HttpResponse::createTextResponse(const QString &textBody, StatusCode statusCode)
{
    ResponseData response;
    response.statusCode = statusCode;
    response.statusMessage = getStatusMessage(statusCode);
    response.version = "1.1";
    response.body = textBody.toUtf8();

    // Set standard headers for text response
    response.headers["Content-Type"] = "text/plain; charset=utf-8";
    response.headers["Content-Length"] = QString::number(response.body.size());
    response.headers["Server"] = "Qt MCP Plugin HTTP Server";
    response.headers["Connection"] = "close";

    return buildResponse(response);
}

QByteArray HttpResponse::createErrorResponse(StatusCode statusCode, const QString &errorMessage)
{
    // Create JSON error response
    QString jsonError = QString(R"({"error":{"code":%1,"message":"%2"}})")
                            .arg(static_cast<int>(statusCode))
                            .arg(errorMessage);

    ResponseData response;
    response.statusCode = statusCode;
    response.statusMessage = getStatusMessage(statusCode);
    response.version = "1.1";
    response.body = jsonError.toUtf8();

    // Set standard headers for error response
    response.headers["Content-Type"] = "application/json; charset=utf-8";
    response.headers["Content-Length"] = QString::number(response.body.size());
    response.headers["Server"] = "Qt MCP Plugin HTTP Server";
    response.headers["Connection"] = "close";

    return buildResponse(response);
}

QByteArray HttpResponse::createCorsResponse(const QByteArray &jsonBody, StatusCode statusCode)
{
    ResponseData response;
    response.statusCode = statusCode;
    response.statusMessage = getStatusMessage(statusCode);
    response.version = "1.1";
    response.body = jsonBody;

    // Set standard headers for JSON response
    response.headers["Content-Type"] = "application/json; charset=utf-8";
    response.headers["Content-Length"] = QString::number(jsonBody.size());
    response.headers["Server"] = "Qt MCP Plugin HTTP Server";
    response.headers["Connection"] = "close";

    // Add CORS headers
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    response.headers["Access-Control-Max-Age"] = "3600";

    return buildResponse(response);
}

QByteArray HttpResponse::buildResponse(const ResponseData &response)
{
    QString httpResponse;
    QTextStream stream(&httpResponse);

    // Status line: HTTP/1.1 200 OK
    stream << formatStatusLine(response.version, response.statusCode, response.statusMessage)
           << "\r\n";

    // Headers
    stream << formatHeaders(response.headers) << "\r\n";

    // Body
    if (!response.body.isEmpty()) {
        stream << response.body;
    }

    return httpResponse.toUtf8();
}

QString HttpResponse::getStatusMessage(StatusCode statusCode)
{
    switch (statusCode) {
    case OK:
        return "OK";
    case CREATED:
        return "Created";
    case NO_CONTENT:
        return "No Content";
    case BAD_REQUEST:
        return "Bad Request";
    case UNAUTHORIZED:
        return "Unauthorized";
    case FORBIDDEN:
        return "Forbidden";
    case NOT_FOUND:
        return "Not Found";
    case METHOD_NOT_ALLOWED:
        return "Method Not Allowed";
    case INTERNAL_SERVER_ERROR:
        return "Internal Server Error";
    case NOT_IMPLEMENTED:
        return "Not Implemented";
    case BAD_GATEWAY:
        return "Bad Gateway";
    case SERVICE_UNAVAILABLE:
        return "Service Unavailable";
    default:
        return "Unknown Status";
    }
}

QString HttpResponse::formatStatusLine(
    const QString &version, StatusCode statusCode, const QString &statusMessage)
{
    return QString("HTTP/%1 %2 %3").arg(version).arg(static_cast<int>(statusCode)).arg(statusMessage);
}

QString HttpResponse::formatHeaders(const QMap<QString, QString> &headers)
{
    QString headerString;
    QTextStream stream(&headerString);

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        stream << it.key() << ": " << it.value() << "\r\n";
    }

    return headerString;
}

} // namespace Internal
} // namespace Mcp
