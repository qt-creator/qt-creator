// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldebugtranslationclient.h"
#include <qmldebug/qpacketprotocol.h>

#include <QUrl>

#include <private/qqmldebugtranslationprotocol_p.h>

namespace QmlPreview {

QmlDebugTranslationClient::QmlDebugTranslationClient(QmlDebug::QmlDebugConnection *connection) :
    QmlDebug::QmlDebugClient(QLatin1String("DebugTranslation"), connection)
{
}

void QmlDebugTranslationClient::changeLanguage(const QUrl &url, const QString &localeIsoCode)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    const int request_change_language = 1;
    packet << request_change_language << url << localeIsoCode;
    sendMessage(packet.data());
}

void QmlDebugTranslationClient::stateChanged(QmlDebug::QmlDebugClient::State state)
{
    if (state == Unavailable)
        emit debugServiceUnavailable();
}

} // namespace QmlPreview
