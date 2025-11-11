// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiapiutils.h"

#include "airesponse.h"

#include <QBuffer>
#include <QImage>

namespace QmlDesigner::AiApiUtils {

namespace {

QString removeThinkBlock(const QString &str)
{
    QString data = str;
    if (data.startsWith("<think>")) {
        int endPos = data.indexOf("</think>");
        if (endPos != -1)
            data.remove(0, endPos + 8); // 8 is length of "</think>"
        else                            // If no closing tag, remove the opening tag only
            data.remove(0, 7);          // 7 is length of "<think>"
    }
    return data;
}

QString unwrapQmlQuote(const QString &str)
{
    if (str.startsWith("```qml", Qt::CaseInsensitive) && str.endsWith("```"))
        return str.mid(6, str.size() - 9).trimmed();
    return str;
}

} // namespace

QString toBase64Image(const QImage &image, const QByteArray &format)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    image.save(&buffer, format);
    return QString::fromLatin1(byteArray.toBase64());
}

AiResponse aiResponseFromContent(const QString &contentStr)
{
    QString content = contentStr.trimmed();
    content = removeThinkBlock(content);
    content = unwrapQmlQuote(content);

    QStringList selectedIds;
    QString qml;

    if (content.startsWith("$$")) {
        QString inner = content.mid(2);

        if (inner.endsWith("$$"))
            inner.chop(2);

        selectedIds = inner.split(",", Qt::SkipEmptyParts);
        for (QString &id : selectedIds)
            id = id.trimmed();
    } else {
        qml = content;
    }

    AiResponse result;
    result.setQml(qml);
    result.setSelectedIds(selectedIds);
    return result;
}

} // namespace QmlDesigner::AiApiUtils
