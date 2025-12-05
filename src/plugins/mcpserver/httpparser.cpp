// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "httpparser.h"

#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTextStream>

Q_LOGGING_CATEGORY(mcpHttpParser, "qtc.mcpserver.httpparser", QtWarningMsg)

namespace Mcp {
namespace Internal {

HttpParser::HttpParser(QObject *parent)
    : QObject(parent)
{}

HttpParser::HttpRequest HttpParser::parseRequest(const QByteArray &data)
{
    HttpRequest request;
    request.isValid = false;
    request.needMoreData = false;
    request.errorMessage.clear();

    if (data.isEmpty()) {
        request.errorMessage = "Empty request data";
        return request;
    }

    // Split data into lines
    QStringList lines = splitHttpLines(data);
    if (lines.isEmpty()) {
        request.errorMessage = "No data lines found";
        return request;
    }

    // Parse request line (first line)
    if (!parseRequestLine(lines[0], request)) {
        request.errorMessage = "Invalid request line";
        return request;
    }

    // Find end of headers by looking for empty line or header termination
    int headerEndIndex = -1;

    // Method 1: Look for empty line in normalized lines
    for (int i = 1; i < lines.size(); ++i) {
        if (lines[i].trimmed().isEmpty()) {
            headerEndIndex = i;
            break;
        }
    }

    // Method 2: If no empty line found, look for header termination in raw data
    if (headerEndIndex == -1) {
        // Try different termination patterns
        QList<QByteArray> terminators = {"\r\n\r\n", "\n\n", "\r\r"};
        int terminatorPos = -1;

        for (const QByteArray &terminator : terminators) {
            terminatorPos = data.indexOf(terminator);
            if (terminatorPos != -1) {
                break;
            }
        }

        if (terminatorPos == -1) {
            // Method 3: If still no termination found, assume all lines are headers
            // This handles cases where the request doesn't have a body
            headerEndIndex = lines.size();
        } else {
            // Found termination - calculate which line it corresponds to
            QByteArray headerSection = data.left(terminatorPos);
            QString headerStr = QString::fromUtf8(headerSection);
            headerStr = headerStr.replace("\r\n", "\n").replace("\r", "\n");
            QStringList headerLines = headerStr.split("\n", Qt::KeepEmptyParts);
            headerEndIndex = headerLines.size();
        }
    }

    // Parse headers (lines 1 to headerEndIndex-1)
    QStringList headerLines;
    for (int i = 1; i < headerEndIndex; ++i) {
        headerLines.append(lines[i]);
    }
    parseHeaders(headerLines, request);

    // Extract body if present
    int contentLength = 0;
    if (request.headers.contains("content-length")) {
        bool ok;
        contentLength = request.headers["content-length"].toInt(&ok);
        if (!ok) {
            contentLength = 0;
        }
    }

    if (contentLength > 0) {
        // Find where headers end in raw data using multiple termination patterns
        QList<QByteArray> terminators = {"\r\n\r\n", "\n\n", "\r\r"};
        int headerTerminatorPos = -1;
        int terminatorLength = 0;

        for (const QByteArray &terminator : terminators) {
            int pos = data.indexOf(terminator);
            if (pos != -1) {
                headerTerminatorPos = pos;
                terminatorLength = terminator.size();
                break;
            }
        }

        if (headerTerminatorPos == -1) {
            // No header termination found - this might be a request without proper headers
            // Try to extract body from the end of the data
            if (data.size() >= contentLength) {
                request.body = data.right(contentLength);
            } else {
                request.body = data; // Use all remaining data
            }
        } else {
            int headerEnd = headerTerminatorPos + terminatorLength;
            request.body = extractBody(data, headerEnd, contentLength);
        }

        if (request.body.size() != contentLength) {
            request.needMoreData = true;
            request.errorMessage = QString("Body incomplete: %1/%2 bytes received")
                                       .arg(request.body.size()).arg(contentLength);
            // Keep the partially‑read body; the caller can prepend it to the next read.
            return request;
        }
    }

    request.isValid = true;
    return request;
}

bool HttpParser::isHttpRequest(const QByteArray &data)
{
    if (data.isEmpty()) {
        return false;
    }

    // Convert to string for analysis
    QString dataStr = QString::fromUtf8(data.left(100)); // Check first 100 characters

    // Method 1: Check if data starts with HTTP method
    QStringList httpMethods
        = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"};

    for (const QString &method : httpMethods) {
        if (dataStr.startsWith(method + " ")) {
            return true;
        }
    }

    // Method 2: Check for HTTP version in the first line
    if (dataStr.contains("HTTP/1.") || dataStr.contains("HTTP/2.")) {
        return true;
    }

    // Method 3: Check for common HTTP headers
    QStringList httpHeaders
        = {"host:",
           "user-agent:",
           "content-type:",
           "content-length:",
           "accept:",
           "connection:",
           "cache-control:"};

    QString lowerDataStr = dataStr.toLower();
    for (const QString &header : httpHeaders) {
        if (lowerDataStr.contains(header)) {
            return true;
        }
    }

    return false;
}

bool HttpParser::parseRequestLine(const QString &requestLine, HttpRequest &request)
{
    // HTTP request line format: METHOD URI HTTP/VERSION
    // Example: "GET /api/test HTTP/1.1"
    // Be flexible with whitespace and handle various formats

    QString trimmedLine = requestLine.trimmed();

    // More flexible regex that handles extra whitespace
    QRegularExpression regex(R"(^([A-Z]+)\s+(.+?)\s+HTTP/(\d+\.\d+)$)");
    QRegularExpressionMatch match = regex.match(trimmedLine);

    if (!match.hasMatch()) {
        // Try alternative parsing for malformed requests
        QStringList parts = trimmedLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            request.method = parts[0];
            request.uri = parts[1];

            // Extract version from the last part
            QString versionPart = parts.last();
            QRegularExpression versionRegex(R"(HTTP/(\d+\.\d+))");
            QRegularExpressionMatch versionMatch = versionRegex.match(versionPart);
            if (versionMatch.hasMatch()) {
                request.version = versionMatch.captured(1);
            } else {
                qCDebug(mcpHttpParser) << "Could not parse HTTP version from:" << versionPart;
                return false;
            }
        } else {
            qCDebug(mcpHttpParser) << "Invalid request line format:" << requestLine;
            return false;
        }
    } else {
        request.method = match.captured(1);
        request.uri = match.captured(2).trimmed(); // Trim URI as well
        request.version = match.captured(3);
    }

    // Validate HTTP method
    QStringList validMethods
        = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"};
    if (!validMethods.contains(request.method)) {
        qCDebug(mcpHttpParser) << "Unsupported HTTP method:" << request.method;
        return false;
    }

    // Validate HTTP version (be more permissive)
    if (!request.version.startsWith("1.") && !request.version.startsWith("2.")) {
        qCDebug(mcpHttpParser) << "Unsupported HTTP version:" << request.version;
        return false;
    }

    qCDebug(mcpHttpParser) << "Parsed request line:" << request.method << request.uri << "HTTP/"
             << request.version;
    return true;
}

void HttpParser::parseHeaders(const QStringList &headerLines, HttpRequest &request)
{
    // Parse each header line: "Name: Value"
    // Handle continuation lines and malformed headers gracefully
    for (const QString &line : headerLines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) {
            continue; // Skip empty lines
        }

        // Handle header continuation (lines starting with space or tab)
        if (trimmedLine.startsWith(' ') || trimmedLine.startsWith('\t')) {
            // This is a continuation of the previous header
            if (!request.headers.isEmpty()) {
                QString lastKey = request.headers.keys().last();
                request.headers[lastKey] += " " + trimmedLine.trimmed();
                continue;
            }
        }

        int colonPos = trimmedLine.indexOf(':');
        if (colonPos == -1) {
            qCDebug(mcpHttpParser) << "Invalid header line (no colon):" << trimmedLine;
            continue;
        }

        QString headerName = trimmedLine.left(colonPos).trimmed();
        QString headerValue = trimmedLine.mid(colonPos + 1).trimmed();

        // Skip empty header names
        if (headerName.isEmpty()) {
            qCDebug(mcpHttpParser) << "Empty header name in line:" << trimmedLine;
            continue;
        }

        // Normalize header name for case-insensitive lookup
        QString normalizedName = normalizeHeaderName(headerName);
        request.headers[normalizedName] = headerValue;

        qCDebug(mcpHttpParser) << "Parsed header:" << normalizedName << "=" << headerValue;
    }
}

QString HttpParser::normalizeHeaderName(const QString &headerName)
{
    return headerName.toLower();
}

QByteArray HttpParser::extractBody(const QByteArray &data, int headerEnd, int contentLength)
{
    if (contentLength <= 0) {
        return QByteArray();
    }

    // Extract body content after headers
    QByteArray body = data.mid(headerEnd, contentLength);
    return body;
}

QStringList HttpParser::splitHttpLines(const QByteArray &data)
{
    QStringList lines;

    // Convert to string and handle different line ending formats
    QString dataStr = QString::fromUtf8(data);

    // Normalize line endings - handle \r\n, \n, and \r
    dataStr = dataStr.replace("\r\n", "\n").replace("\r", "\n");
    lines = dataStr.split("\n", Qt::KeepEmptyParts);

    // Remove empty trailing elements
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
        lines.removeLast();
    }

    return lines;
}

} // namespace Internal
} // namespace Mcp
