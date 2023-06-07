// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
                                  [[maybe_unused]] AbstractView *view,
                                  ExternalDependenciesInterface &)
{
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
        if (command.typeId() != QMetaType::type("PuppetAliveCommand")) {
            if (command.typeId() == QMetaType::type("SyncNanotraceCommand")) {
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

