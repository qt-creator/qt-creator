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

#include "connectionmanager.h"
#include "endpuppetcommand.h"
#include "nodeinstanceserverproxy.h"
#include "nodeinstanceview.h"
#include "puppetcreator.h"

#ifndef QMLDESIGNER_TEST
#include <qmldesignerplugin.h>
#endif

#include <projectexplorer/target.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QUuid>

namespace QmlDesigner {

ConnectionManager::ConnectionManager() = default;

ConnectionManager::~ConnectionManager() = default;

void ConnectionManager::setUp(NodeInstanceServerProxy *nodeInstanceServerProxy,
                              const QString &qrcMappingString,
                              ProjectExplorer::Target *target)
{
    BaseConnectionManager::setUp(nodeInstanceServerProxy, qrcMappingString, target);

    m_localServer = std::make_unique<QLocalServer>();
    QString socketToken(QUuid::createUuid().toString());
    m_localServer->listen(socketToken);
    m_localServer->setMaxPendingConnections(3);

    NodeInstanceView *nodeInstanceView = nodeInstanceServerProxy->nodeInstanceView();
    PuppetCreator puppetCreator(target, nodeInstanceView->model());
    puppetCreator.setQrcMappingString(qrcMappingString);

    puppetCreator.createQml2PuppetExecutableIfMissing();

    for (Connection &connection : m_connections) {
        connection.qmlPuppetProcess = puppetCreator.createPuppetProcess(
            connection.mode,
            socketToken,
            nodeInstanceView,
            [&] { printProcessOutput(connection.qmlPuppetProcess.get(), connection.name); },
            [&](int exitCode, QProcess::ExitStatus exitStatus) {
                processFinished(exitCode, exitStatus);
            });

        const int second = 1000;
        int waitConstant = 8 * second;

        if (!connection.qmlPuppetProcess->waitForStarted(waitConstant)) {
            closeSocketsAndKillProcesses();
            showCannotConnectToPuppetWarningAndSwitchToEditMode();
            return;
        }

        waitConstant /= 2;

        bool connectedToPuppet = true;
        if (!m_localServer->hasPendingConnections())
            connectedToPuppet = m_localServer->waitForNewConnection(waitConstant);

        if (connectedToPuppet) {
            connection.socket.reset(m_localServer->nextPendingConnection());
            QObject::connect(connection.socket.get(), &QIODevice::readyRead, [&] {
                readDataStream(connection);
            });
        } else {
            closeSocketsAndKillProcesses();
            showCannotConnectToPuppetWarningAndSwitchToEditMode();
            return;
        }
    }

    m_localServer->close();

    connect(this,
            &ConnectionManager::processCrashed,
            nodeInstanceServerProxy,
            &NodeInstanceServerProxy::processCrashed);
}

void ConnectionManager::shutDown()
{
    BaseConnectionManager::shutDown();

    closeSocketsAndKillProcesses();

    m_localServer.reset();

    for (Connection &connection : m_connections)
        connection.clear();
}

void ConnectionManager::writeCommand(const QVariant &command)
{
    for (Connection &connection : m_connections)
        writeCommandToIODevice(command, connection.socket.get(), m_writeCommandCounter);

    m_writeCommandCounter++;
}

void ConnectionManager::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    auto finishedProcess = qobject_cast<QProcess *>(sender());
    if (finishedProcess)
        qWarning() << "Process" << (exitStatus == QProcess::CrashExit ? "crashed:" : "finished:")
                   << finishedProcess->arguments() << "exitCode:" << exitCode;
    else
        qWarning() << "Process" << (exitStatus == QProcess::CrashExit ? "crashed:" : "finished:")
                   << sender() << "exitCode:" << exitCode;

    writeCommand(QVariant::fromValue(EndPuppetCommand()));

    closeSocketsAndKillProcesses();

    if (exitStatus == QProcess::CrashExit)
        emit processCrashed();
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

        if (connection.qmlPuppetProcess) {
            QTimer::singleShot(3000, connection.qmlPuppetProcess.get(), &QProcess::terminate);
            QTimer::singleShot(6000, connection.qmlPuppetProcess.get(), &QProcess::kill);
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
