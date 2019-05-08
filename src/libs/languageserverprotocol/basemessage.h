/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "languageserverprotocol_global.h"

#include <utils/mimetypes/mimetype.h>

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
    Q_DECLARE_TR_FUNCTIONS(BaseMessage)
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
    QByteArray toData() const;

    QByteArray mimeType;
    QByteArray content;
    int contentLength = -1;
    QTextCodec *codec = defaultCodec();

private:
    QByteArray header() const;
    QByteArray lengthHeader() const;
    QByteArray typeHeader() const;
};

} // namespace LanguageClient
