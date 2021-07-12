/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "launchersockethandler.h"

#include "launcherlogging.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtNetwork/qlocalsocket.h>

namespace Utils {
namespace Internal {

class Process : public QProcess
{
    Q_OBJECT
public:
    Process(quintptr token, QObject *parent = nullptr) :
        QProcess(parent), m_token(token), m_stopTimer(new QTimer(this))
    {
        m_stopTimer->setSingleShot(true);
        connect(m_stopTimer, &QTimer::timeout, this, &Process::cancel);
    }

    void cancel()
    {
        switch (m_stopState) {
        case StopState::Inactive:
            m_stopState = StopState::Terminating;
            m_stopTimer->start(3000);
            terminate();
            break;
        case StopState::Terminating:
            m_stopState = StopState::Killing;
            m_stopTimer->start(3000);
            kill();
            break;
        case StopState::Killing:
            m_stopState = StopState::Inactive;
            emit failedToStop();
            break;
        }
    }

    void stopStopProcedure()
    {
        m_stopState = StopState::Inactive;
        m_stopTimer->stop();
    }

    quintptr token() const { return m_token; }

signals:
    void failedToStop();

private:
    const quintptr m_token;
    QTimer * const m_stopTimer;
    enum class StopState { Inactive, Terminating, Killing } m_stopState = StopState::Inactive;
};

LauncherSocketHandler::LauncherSocketHandler(QString serverPath, QObject *parent)
    : QObject(parent),
      m_serverPath(std::move(serverPath)),
      m_socket(new QLocalSocket(this))
{
    m_packetParser.setDevice(m_socket);
}

LauncherSocketHandler::~LauncherSocketHandler()
{
    m_socket->disconnect();
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        logWarn("socket handler destroyed while connection was active");
        m_socket->close();
    }
    for (auto it = m_processes.cbegin(); it != m_processes.cend(); ++it)
        it.value()->disconnect();
}

void LauncherSocketHandler::start()
{
    connect(m_socket, &QLocalSocket::disconnected,
            this, &LauncherSocketHandler::handleSocketClosed);
    connect(m_socket, &QLocalSocket::readyRead, this, &LauncherSocketHandler::handleSocketData);
    connect(m_socket,
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
            static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),
#else
            &QLocalSocket::errorOccurred,
#endif
            this, &LauncherSocketHandler::handleSocketError);
    m_socket->connectToServer(m_serverPath);
}

void LauncherSocketHandler::handleSocketData()
{
    try {
        if (!m_packetParser.parse())
            return;
    } catch (const PacketParser::InvalidPacketSizeException &e) {
        logWarn(QStringLiteral("Internal protocol error: invalid packet size %1.")
                .arg(e.size));
        return;
    }
    switch (m_packetParser.type()) {
    case LauncherPacketType::StartProcess:
        handleStartPacket();
        break;
    case LauncherPacketType::StopProcess:
        handleStopPacket();
        break;
    case LauncherPacketType::Shutdown:
        handleShutdownPacket();
        return;
    default:
        logWarn(QStringLiteral("Internal protocol error: invalid packet type %1.")
                .arg(static_cast<int>(m_packetParser.type())));
        return;
    }
    handleSocketData();
}

void LauncherSocketHandler::handleSocketError()
{
    if (m_socket->error() != QLocalSocket::PeerClosedError) {
        logError(QStringLiteral("socket error: %1").arg(m_socket->errorString()));
        m_socket->disconnect();
        qApp->quit();
    }
}

void LauncherSocketHandler::handleSocketClosed()
{
    for (auto it = m_processes.cbegin(); it != m_processes.cend(); ++it) {
        if (it.value()->state() != QProcess::NotRunning) {
            logWarn("client closed connection while process still running");
            break;
        }
    }
    m_socket->disconnect();
    qApp->quit();
}

void LauncherSocketHandler::handleProcessError()
{
    Process * proc = senderProcess();
    if (proc->error() != QProcess::FailedToStart)
        return;
    proc->stopStopProcedure();
    ProcessErrorPacket packet(proc->token());
    packet.error = proc->error();
    packet.errorString = proc->errorString();
    sendPacket(packet);
}

void LauncherSocketHandler::handleProcessStarted()
{
    Process *proc = senderProcess();
    ProcessStartedPacket packet(proc->token());
    packet.processId = proc->processId();
    sendPacket(packet);
}

void LauncherSocketHandler::handleProcessFinished()
{
    Process * proc = senderProcess();
    proc->stopStopProcedure();
    ProcessFinishedPacket packet(proc->token());
    packet.error = proc->error();
    packet.errorString = proc->errorString();
    packet.exitCode = proc->exitCode();
    packet.exitStatus = proc->exitStatus();
    packet.stdErr = proc->readAllStandardError();
    packet.stdOut = proc->readAllStandardOutput();
    sendPacket(packet);
}

void LauncherSocketHandler::handleStopFailure()
{
    // Process did not react to a kill signal. Rare, but not unheard of.
    // Forget about the associated Process object and report process exit to the client.
    Process * proc = senderProcess();
    proc->disconnect();
    m_processes.remove(proc->token());
    ProcessFinishedPacket packet(proc->token());
    packet.error = QProcess::Crashed;
    packet.exitCode = -1;
    packet.exitStatus = QProcess::CrashExit;
    packet.stdErr = proc->readAllStandardError();
    packet.stdOut = proc->readAllStandardOutput();
    sendPacket(packet);
}

void LauncherSocketHandler::handleStartPacket()
{
    Process *& process = m_processes[m_packetParser.token()];
    if (!process)
        process = setupProcess(m_packetParser.token());
    if (process->state() != QProcess::NotRunning) {
        logWarn("got start request while process was running");
        return;
    }
    const auto packet = LauncherPacket::extractPacket<StartProcessPacket>(
                m_packetParser.token(),
                m_packetParser.packetData());
    process->setEnvironment(packet.env);
    process->setWorkingDirectory(packet.workingDir);
    process->setProcessChannelMode(packet.mode);
    process->start(packet.command, packet.arguments);
}

void LauncherSocketHandler::handleStopPacket()
{
    Process * const process = m_processes.value(m_packetParser.token());
    if (!process) {
        logWarn("got stop request for unknown process");
        return;
    }
    if (process->state() == QProcess::NotRunning) {
        // This can happen if the process finishes on its own at about the same time the client
        // sends the request.
        logDebug("got stop request when process was not running");
        return;
    }
    process->cancel();
}

void LauncherSocketHandler::handleShutdownPacket()
{
    logDebug("got shutdown request, closing down");
    for (auto it = m_processes.cbegin(); it != m_processes.cend(); ++it) {
        it.value()->disconnect();
        if (it.value()->state() != QProcess::NotRunning) {
            logWarn("got shutdown request while process was running");
            it.value()->terminate();
        }
    }
    m_socket->disconnect();
    qApp->quit();
}

void LauncherSocketHandler::sendPacket(const LauncherPacket &packet)
{
    m_socket->write(packet.serialize());
}

Process *LauncherSocketHandler::setupProcess(quintptr token)
{
    const auto p = new Process(token, this);
    connect(p, &QProcess::errorOccurred, this, &LauncherSocketHandler::handleProcessError);
    connect(p, &QProcess::started, this, &LauncherSocketHandler::handleProcessStarted);
    connect(p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &LauncherSocketHandler::handleProcessFinished);
    connect(p, &Process::failedToStop, this, &LauncherSocketHandler::handleStopFailure);
    return p;
}

Process *LauncherSocketHandler::senderProcess() const
{
    return static_cast<Process *>(sender());
}

} // namespace Internal
} // namespace Utils

#include <launchersockethandler.moc>
