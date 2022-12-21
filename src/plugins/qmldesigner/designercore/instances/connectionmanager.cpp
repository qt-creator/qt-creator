// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "connectionmanager.h"
#include "endpuppetcommand.h"
#include "puppetstarter.h"

#include <externaldependenciesinterface.h>
#include <abstractview.h>

#include <projectexplorer/target.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QUuid>

namespace QmlDesigner {

ConnectionManager::ConnectionManager() = default;

ConnectionManager::~ConnectionManager() = default;

void ConnectionManager::setUp(NodeInstanceServerInterface *nodeInstanceServerProxy,
                              const QString &qrcMappingString,
                              ProjectExplorer::Target *target,
                              AbstractView *view,
                              ExternalDependenciesInterface &externalDependencies)
{
    BaseConnectionManager::setUp(nodeInstanceServerProxy,
                                 qrcMappingString,
                                 target,
                                 view,
                                 externalDependencies);

    for (Connection &connection : m_connections) {

        QString socketToken(QUuid::createUuid().toString());
        connection.localServer = std::make_unique<QLocalServer>();
        connection.localServer->listen(socketToken);
        connection.localServer->setMaxPendingConnections(1);

        connection.qmlPuppetProcess = PuppetStarter::createPuppetProcess(
            externalDependencies.puppetStartData(*view->model()),
            connection.mode,
            socketToken,
            [&] { printProcessOutput(connection.qmlPuppetProcess.get(), connection.name); },
            [&](int exitCode, QProcess::ExitStatus exitStatus) {
                processFinished(exitCode, exitStatus, connection.name);
            });
    }

    const int second = 1000;
    for (Connection &connection : m_connections) {
        int waitConstant = 8 * second;
        if (!connection.qmlPuppetProcess->waitForStarted(waitConstant)) {
            closeSocketsAndKillProcesses();
            showCannotConnectToPuppetWarningAndSwitchToEditMode();
            return;
        }

        waitConstant /= 2;

        bool connectedToPuppet = true;
        if (!connection.localServer->hasPendingConnections())
            connectedToPuppet = connection.localServer->waitForNewConnection(waitConstant);

        if (connectedToPuppet) {
            connection.socket.reset(connection.localServer->nextPendingConnection());
            QObject::connect(connection.socket.get(), &QIODevice::readyRead, this, [&] {
                readDataStream(connection);
            });
        } else {
            closeSocketsAndKillProcesses();
            showCannotConnectToPuppetWarningAndSwitchToEditMode();
            return;
        }
        connection.localServer->close();
    }
}

void ConnectionManager::shutDown()
{
    BaseConnectionManager::shutDown();

    closeSocketsAndKillProcesses();
}

void ConnectionManager::writeCommand(const QVariant &command)
{
    for (Connection &connection : m_connections)
        writeCommandToIODevice(command, connection.socket.get(), m_writeCommandCounter);

    m_writeCommandCounter++;
}

quint32 ConnectionManager::writeCounter() const
{
    return m_writeCommandCounter;
}

void ConnectionManager::processFinished(int exitCode, QProcess::ExitStatus exitStatus, const QString &connectionName)
{
    qWarning() << "Process" << connectionName <<(exitStatus == QProcess::CrashExit ? "crashed:" : "finished:")
               << "with exitCode:" << exitCode;

    writeCommand(QVariant::fromValue(EndPuppetCommand()));

    closeSocketsAndKillProcesses();

    if (exitStatus == QProcess::CrashExit)
        callCrashCallback();
}

void ConnectionManager::closeSocketsAndKillProcesses()
{
    for (Connection &connection : m_connections) {
        if (connection.socket) {
            disconnect(connection.socket.get());
            disconnect(connection.qmlPuppetProcess.get());
            connection.socket->waitForBytesWritten(1000);
            connection.socket->abort();
        }

        connection.clear();
    }
}

void ConnectionManager::printProcessOutput(QProcess *process, const QString &connectionName)
{
    while (process && process->canReadLine()) {
        QByteArray line = process->readLine();
        line.chop(1);
        qDebug().nospace() << connectionName << " Puppet: " << line;
    }
    qDebug() << "\n";
}

} // namespace QmlDesigner
