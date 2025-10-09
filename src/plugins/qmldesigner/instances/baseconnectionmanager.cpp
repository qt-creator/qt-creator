// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baseconnectionmanager.h"
#include "endpuppetcommand.h"
#include "nanotrace/nanotrace.h"
#include "nanotracecommand.h"
#include "nodeinstanceserverproxy.h"
#include "nodeinstancetracing.h"
#include "nodeinstanceview.h"

#include <QLocalSocket>
#include <QTimer>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using NodeInstanceTracing::category;

void BaseConnectionManager::setUp(NodeInstanceServerInterface *nodeInstanceServer,
                                  const QString &,
                                  ProjectExplorer::Target *,
                                  [[maybe_unused]] AbstractView *view,
                                  ExternalDependenciesInterface &)
{
    NanotraceHR::Tracer tracer{"base connection manager setup", category()};

    m_nodeInstanceServer = nodeInstanceServer;
    m_isActive = true;
}

void BaseConnectionManager::shutDown()
{
    NanotraceHR::Tracer tracer{"base connection manager shutdown", category()};

    m_isActive = false;

    writeCommand(QVariant::fromValue(EndPuppetCommand()));

    m_nodeInstanceServer = nullptr;
}

void BaseConnectionManager::setCrashCallback(std::function<void()> callback)
{
    NanotraceHR::Tracer tracer{"base connection manager set crash callback", category()};

    std::lock_guard<std::mutex> lock{m_callbackMutex};

    m_crashCallback = std::move(callback);
}

bool BaseConnectionManager::isActive() const
{
    NanotraceHR::Tracer tracer{"base connection manager is active", category()};

    return m_isActive;
}

void BaseConnectionManager::showCannotConnectToPuppetWarningAndSwitchToEditMode(
    const QString & /*qmlPuppetPath*/)
{
    NanotraceHR::Tracer tracer{"base connection manager show cannot connect warning", category()};
}

void BaseConnectionManager::processFinished(const QString &reason)
{
    NanotraceHR::Tracer tracer{"base connection manager process finished",
                               category(),
                               keyValue("reason", reason)};

    processFinished(-1, QProcess::CrashExit, reason);
}

void BaseConnectionManager::writeCommandToIODevice(const QVariant &command,
                                                   QIODevice *ioDevice,
                                                   unsigned int commandCounter)
{
    NanotraceHR::Tracer tracer{"base connection manager write command to io device", category()};

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
    NanotraceHR::Tracer tracer{"base connection manager dispatch command", category()};

    if (!isActive())
        return;

    m_nodeInstanceServer->dispatchCommand(command);
}

void BaseConnectionManager::readDataStream(Connection &connection)
{
    NanotraceHR::Tracer tracer{"base connection manager read data stream", category()};

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

#ifdef NANOTRACE_DESIGNSTUDIO_ENABLED
        if (command.typeId() != QMetaType::fromName("PuppetAliveCommand").id()) {
            if (command.typeId() == QMetaType::fromName("SyncNanotraceCommand").id()) {
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
    NanotraceHR::Tracer tracer{"base connection manager call crash callback", category()};

    std::lock_guard<std::mutex> lock{m_callbackMutex};

    if (m_crashCallback)
        m_crashCallback();
}
} // namespace QmlDesigner

