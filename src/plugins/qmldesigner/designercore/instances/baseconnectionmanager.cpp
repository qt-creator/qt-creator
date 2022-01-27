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

#include "baseconnectionmanager.h"
#include "endpuppetcommand.h"
#include "nodeinstanceserverproxy.h"
#include "nodeinstanceview.h"
#include "nanotracecommand.h"
#include "nanotrace/nanotrace.h"

#include <QLocalSocket>
#include <QTimer>

namespace QmlDesigner {

void BaseConnectionManager::setUp(NodeInstanceServerInterface *nodeInstanceServer,
                                  const QString &,
                                  ProjectExplorer::Target *,
                                  AbstractView *view)
{
    Q_UNUSED(view)
    m_nodeInstanceServer = nodeInstanceServer;
    m_isActive = true;
}

void BaseConnectionManager::shutDown()
{
    m_isActive = false;

    writeCommand(QVariant::fromValue(EndPuppetCommand()));

    m_nodeInstanceServer = nullptr;
}

void BaseConnectionManager::setCrashCallback(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock{m_callbackMutex};

    m_crashCallback = std::move(callback);
}

bool BaseConnectionManager::isActive() const
{
    return m_isActive;
}

void BaseConnectionManager::showCannotConnectToPuppetWarningAndSwitchToEditMode() {}

void BaseConnectionManager::processFinished(const QString &reason)
{
    processFinished(-1, QProcess::CrashExit, reason);
}

void BaseConnectionManager::writeCommandToIODevice(const QVariant &command,
                                                   QIODevice *ioDevice,
                                                   unsigned int commandCounter)
{
    if (ioDevice) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_8);
        out << quint32(0);
        out << quint32(commandCounter);
        out << command;
        out.device()->seek(0);
        out << quint32(static_cast<unsigned long long>(block.size()) - sizeof(quint32));

        ioDevice->write(block);
    }
}

void BaseConnectionManager::dispatchCommand(const QVariant &command, Connection &)
{
    if (!isActive())
        return;

    m_nodeInstanceServer->dispatchCommand(command);
}

void BaseConnectionManager::readDataStream(Connection &connection)
{
    QList<QVariant> commandList;

    while (!connection.socket->atEnd()) {
        if (connection.socket->bytesAvailable() < int(sizeof(quint32)))
            break;

        QDataStream in(connection.socket.get());
        in.setVersion(QDataStream::Qt_4_8);

        if (connection.blockSize == 0)
            in >> connection.blockSize;

        if (connection.socket->bytesAvailable() < connection.blockSize)
            break;

        quint32 commandCounter = 0;
        in >> commandCounter;
        bool commandLost = !((connection.lastReadCommandCounter == 0 && commandCounter == 0)
                             || (connection.lastReadCommandCounter + 1 == commandCounter));
        if (commandLost)
            qDebug() << "server command lost: " << connection.lastReadCommandCounter << commandCounter;
        connection.lastReadCommandCounter = commandCounter;

        QVariant command;
        in >> command;
        connection.blockSize = 0;

#ifdef NANOTRACE_ENABLED
        if (command.userType() != QMetaType::type("PuppetAliveCommand")) {
            if (command.userType() == QMetaType::type("SyncNanotraceCommand")) {
                SyncNanotraceCommand cmd = command.value<SyncNanotraceCommand>();
                NANOTRACE_INSTANT_ARGS("Sync", "readCommand",
                    {"name", cmd.name().toStdString()},
                    {"counter", int64_t(commandCounter)});

                writeCommand(command);
                // Do not dispatch this command.
                continue;

            } else {
                NANOTRACE_INSTANT_ARGS("Update", "readCommand",
                    {"name", command.typeName()},
                    {"counter", int64_t(commandCounter)});
            }
        }
#endif

        commandList.append(command);
    }

    for (const QVariant &command : commandList)
        dispatchCommand(command, connection);
}

void BaseConnectionManager::callCrashCallback()
{
    std::lock_guard<std::mutex> lock{m_callbackMutex};

    if (m_crashCallback)
        m_crashCallback();
}
} // namespace QmlDesigner

