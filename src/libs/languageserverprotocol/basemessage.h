// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageserverprotocol_global.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE
class QBuffer;
class QTextCodec;
QT_END_NAMESPACE

namespace LanguageServerProtocol {

LANGUAGESERVERPROTOCOL_EXPORT Q_DECLARE_LOGGING_CATEGORY(parseLog)

class LANGUAGESERVERPROTOCOL_EXPORT BaseMessage
{
public:
    BaseMessage();
    BaseMessage(const QByteArray &mimeType, const QByteArray &content,
                int expectedLength, QTextCodec *codec);
    BaseMessage(const QByteArray &mimeType, const QByteArray &content);

    bool operator==(const BaseMessage &other) const;

    static void parse(QBuffer *data, QString &parseError, BaseMessage &message);
    static QTextCodec *defaultCodec();

    bool isComplete() const;
    bool isValid() const;
    QByteArray header() const;

    QByteArray mimeType;
    QByteArray content;
    int contentLength = -1;
    QTextCodec *codec = defaultCodec();

private:
    QByteArray lengthHeader() const;
    QByteArray typeHeader() const;
};

} // namespace LanguageClient
