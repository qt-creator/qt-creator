// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionresultparser.h"

#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Axivion::Internal {

static std::pair<QByteArray, QByteArray> splitHeaderAndBody(const QByteArray &input)
{
    QByteArray header;
    QByteArray json;
    int emptyLine = input.indexOf("\r\n\r\n"); // we always get \r\n as line separator
    if (emptyLine != -1) {
        header = input.left(emptyLine);
        json = input.mid(emptyLine + 4);
    } else {
        json = input;
    }
    return {header, json};
}

static int httpStatus(const QByteArray &header)
{
    int firstHeaderEnd = header.indexOf("\r\n");
    if (firstHeaderEnd == -1)
        return 600; // unexpected header
    const QString firstLine = QString::fromUtf8(header.first(firstHeaderEnd));
    static const QRegularExpression regex(R"(^HTTP/\d\.\d (\d{3}) .*$)");
    const QRegularExpressionMatch match = regex.match(firstLine);
    return match.hasMatch() ? match.captured(1).toInt() : 601;
}

static BaseResult prehandleHeader(const QByteArray &header, const QByteArray &body)
{
    BaseResult result;
    if (header.isEmpty()) {
        result.error = QString::fromUtf8(body); // we likely had a curl problem
        return result;
    }
    int status = httpStatus(header);
    if ((status > 399) || (status > 299 && body.isEmpty())) { // FIXME handle some explicitly?
        const QString statusStr = QString::number(status);
        if (body.isEmpty() || body.startsWith('<')) // likely an html response or redirect
            result.error = QLatin1String("(%1)").arg(statusStr);
        else
            result.error = QLatin1String("%1 (%2)").arg(QString::fromUtf8(body)).arg(statusStr);
    }
    return result;
}

namespace ResultParser {

QString parseRuleInfo(const QByteArray &input) // html result!
{
    auto [header, body] = splitHeaderAndBody(input);
    BaseResult headerResult = prehandleHeader(header, body);
    if (!headerResult.error.isEmpty())
        return QString();
    return QString::fromLocal8Bit(body);
}

} // ResultParser

} // Axivion::Internal
