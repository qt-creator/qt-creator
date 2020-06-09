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
#include <QColor>

namespace QmlPreview {

QmlDebugTranslationClient::QmlDebugTranslationClient(QmlDebug::QmlDebugConnection *connection) :
    QmlDebug::QmlDebugClient(QLatin1String("DebugTranslation"), connection)
{
}

void QmlDebugTranslationClient::changeLanguage(const QUrl &url, const QString &locale)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(ChangeLanguage) << url << locale;
    sendMessage(packet.data());
}

void QmlDebugTranslationClient::changeWarningColor(const QColor &warningColor)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(ChangeWarningColor) << warningColor;
    sendMessage(packet.data());
}

void QmlDebugTranslationClient::changeElidedTextWarningString(const QString &warningString)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(ChangeElidedTextWarningString) << warningString;
    sendMessage(packet.data());
}

void QmlDebugTranslationClient::setDebugTranslationServiceLogFile(const QString &logFilePath)
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(SetDebugTranslationServiceLogFile) << logFilePath;
    sendMessage(packet.data());
}

void QmlDebugTranslationClient::enableElidedTextWarning()
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(EnableElidedTextWarning);
    sendMessage(packet.data());
}

void QmlDebugTranslationClient::disableElidedTextWarning()
{
    QmlDebug::QPacket packet(dataStreamVersion());
    packet << static_cast<qint8>(DisableElidedTextWarning);
    sendMessage(packet.data());
}

void QmlDebugTranslationClient::messageReceived(const QByteArray &data)
{
    QmlDebug::QPacket packet(dataStreamVersion(), data);
    qint8 command;
    packet >> command;
    qDebug() << "invalid command" << command;
}

void QmlDebugTranslationClient::stateChanged(QmlDebug::QmlDebugClient::State state)
{
    if (state == Unavailable)
        emit debugServiceUnavailable();
}

} // namespace QmlPreview
