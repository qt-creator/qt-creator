/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlenginecontrolclient.h"
#include "utils/qtcassert.h"

namespace QmlDebug {

QmlEngineControlClient::QmlEngineControlClient(QmlDebugConnection *client)
    : QmlDebugClient(QLatin1String("EngineControl"), client)
{
}

/*!
 * Block the starting or stopping of the engine with id \a engineId for now. By calling
 * releaseEngine later the block can be lifted again. In the debugged application the engine control
 * server waits until a message is received before continuing. So by not sending a message here we
 * delay the process. Blocks add up. You have to call releaseEngine() as often as you've called
 * blockEngine before. The intention of that is to allow different debug clients to use the same
 * engine control and communicate with their respective counterparts before the QML engine starts or
 * shuts down.
 */
void QmlEngineControlClient::blockEngine(int engineId)
{
    QTC_ASSERT(m_blockedEngines.contains(engineId), return);
    m_blockedEngines[engineId].blockers++;
}

/*!
 * Release the engine with id \a engineId. If no other blocks are present, depending on what the
 * engine is waiting for, the start or stop command is sent to the process being debugged.
 */
void QmlEngineControlClient::releaseEngine(int engineId)
{
    QTC_ASSERT(m_blockedEngines.contains(engineId), return);

    EngineState &state = m_blockedEngines[engineId];
    if (--state.blockers == 0) {
        QTC_ASSERT(state.releaseCommand != InvalidCommand, return);
        sendCommand(state.releaseCommand, engineId);
        m_blockedEngines.remove(engineId);
    }
}

void QmlEngineControlClient::messageReceived(const QByteArray &data)
{
    QDataStream stream(data);
    int message;
    int id;
    QString name;

    stream >> message >> id;

    if (!stream.atEnd())
        stream >> name;

    EngineState &state = m_blockedEngines[id];
    QTC_ASSERT(state.blockers == 0 && state.releaseCommand == InvalidCommand, /**/);

    switch (message) {
    case EngineAboutToBeAdded:
        state.releaseCommand = StartWaitingEngine;
        emit engineAboutToBeAdded(id, name);
        break;
    case EngineAdded:
        emit engineAdded(id, name);
        break;
    case EngineAboutToBeRemoved:
        state.releaseCommand = StopWaitingEngine;
        emit engineAboutToBeRemoved(id, name);
        break;
    case EngineRemoved:
        emit engineRemoved(id, name);
        break;
    }

    if (state.blockers == 0 && state.releaseCommand != InvalidCommand) {
        sendCommand(state.releaseCommand, id);
        m_blockedEngines.remove(id);
    }
}

void QmlEngineControlClient::sendCommand(QmlEngineControlClient::CommandType command, int engineId)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << int(command) << engineId;
    QmlDebugClient::sendMessage(data);
}

}
