/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qmldebugtranslationclient.h"
#include <qmldebug/qpacketprotocol.h>

#include <QUrl>

#ifdef FOUND_QML_DEBUG_TRANSLATION_PROTOCOL
#include <private/qqmldebugtranslationprotocol_p.h>
#endif

namespace QmlPreview {

QmlDebugTranslationClient::QmlDebugTranslationClient(QmlDebug::QmlDebugConnection *connection) :
    QmlDebug::QmlDebugClient(QLatin1String("DebugTranslation"), connection)
{
}

void QmlDebugTranslationClient::changeLanguage(const QUrl &url, const QString &localeIsoCode)
{
    QmlDebug::QPacket packet(dataStreamVersion());
#ifdef FOUND_QML_DEBUG_TRANSLATION_PROTOCOL
    sendMessage(QQmlDebugTranslation::createChangeLanguageRequest(packet, url, localeIsoCode));
#else
    const int request_change_language = 1;
    packet << request_change_language << url << localeIsoCode;
    sendMessage(packet.data());
#endif

}

void QmlDebugTranslationClient::stateChanged(QmlDebug::QmlDebugClient::State state)
{
    if (state == Unavailable)
        emit debugServiceUnavailable();
}

} // namespace QmlPreview
