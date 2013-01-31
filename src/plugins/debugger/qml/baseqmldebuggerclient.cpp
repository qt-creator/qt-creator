/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baseqmldebuggerclient.h"
#include "breakpoint.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

class BaseQmlDebuggerClientPrivate
{
public:
    QList<QByteArray> sendBuffer;
};

BaseQmlDebuggerClient::BaseQmlDebuggerClient(QmlDebug::QmlDebugConnection* client, QLatin1String clientName)
    : QmlDebugClient(clientName, client),
      d(new BaseQmlDebuggerClientPrivate())
{
}

BaseQmlDebuggerClient::~BaseQmlDebuggerClient()
{
    delete d;
}

bool BaseQmlDebuggerClient::acceptsBreakpoint(const BreakpointModelId &/*id*/)
{
    return false;
}

void BaseQmlDebuggerClient::statusChanged(QmlDebug::ClientStatus status)
{
    emit newStatus(status);
}

void BaseQmlDebuggerClient::sendMessage(const QByteArray &msg)
{
    if (status() == QmlDebug::Enabled)
        QmlDebugClient::sendMessage(msg);
    else
        d->sendBuffer.append(msg);
}

void BaseQmlDebuggerClient::flushSendBuffer()
{
    QTC_ASSERT(status() == QmlDebug::Enabled, return);
    foreach (const QByteArray &msg, d->sendBuffer)
       QmlDebugClient::sendMessage(msg);
    d->sendBuffer.clear();
}

} // Internal
} // Debugger
