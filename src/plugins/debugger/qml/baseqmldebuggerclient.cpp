/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baseqmldebuggerclient.h"

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

void BaseQmlDebuggerClient::stateChanged(State state)
{
    emit newState(state);
}

void BaseQmlDebuggerClient::sendMessage(const QByteArray &msg)
{
    if (state() == Enabled)
        QmlDebugClient::sendMessage(msg);
    else
        d->sendBuffer.append(msg);
}

void BaseQmlDebuggerClient::flushSendBuffer()
{
    QTC_ASSERT(state() == Enabled, return);
    foreach (const QByteArray &msg, d->sendBuffer)
       QmlDebugClient::sendMessage(msg);
    d->sendBuffer.clear();
}

} // Internal
} // Debugger
