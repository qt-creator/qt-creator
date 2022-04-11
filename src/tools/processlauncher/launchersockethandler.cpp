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
#include "processreaper.h"
#include "processutils.h"

#include <QCoreApplication>
#include <QLocalSocket>
#include <QProcess>

namespace Utils {
namespace Internal {

class Process : public ProcessHelper
{
    Q_OBJECT
public:
    Process(quintptr token, QObject *parent = nullptr) :
        ProcessHelper(parent), m_token(token) { }

    quintptr token() const { return m_token; }

private:
    const quintptr m_token;
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
    for (auto it = m_processes.cbegin(); it != m_processes.cend(); ++it) {
        Process *p = it.value();
        if (p->state() != QProcess::NotRunning)
            logWarn(QStringLiteral("Shutting down while process %1 is running").arg(p->program()));
        ProcessReaper::reap(p);
    }

    m_socket->disconnect();
    m_socket->disconnectFromServer();
    if (m_socket->state() != QLocalSocket::UnconnectedState
            && !m_socket->waitForDisconnected())  {
        logWarn("Could not disconnect from server");
        m_socket->close();
    }
}

void LauncherSocketHandler::start()
{
    connect(m_socket, &QLocalSocket::disconnected,
            this, &LauncherSocketHandler::handleSocketClosed);
    connect(m_socket, &QLocalSocket::readyRead, this, &LauncherSocketHandler::handleSocketData);
    connect(m_socket,
            &QLocalSocket::errorOccurred,
            this, &LauncherSocketHandler::handleSocketError);
    m_socket->connectToServer(m_serverPath);
}

void LauncherSocketHandler::handleSocketData()
{
    try {
        if (!m_packetParser.parse())
            return;
    } catch (const PacketParser::InvalidPacketSizeException &e) {
        logWarn(QStringLiteral("Internal protocol error: Invalid packet size %1")
                .arg(e.size));
        return;
    }
    switch (m_packetParser.type()) {
    case LauncherPacketType::StartProcess:
        handleStartPacket();
        break;
    case LauncherPacketType::WriteIntoProcess:
        handleWritePacket();
        break;
    case LauncherPacketType::StopProcess:
        handleStopPacket();
        break;
    case LauncherPacketType::Shutdown:
        handleShutdownPacket();
        return;
    default:
        logWarn(QStringLiteral("Internal protocol error: Invalid packet type %1")
                .arg(static_cast<int>(m_packetParser.type())));
        return;
    }
    handleSocketData();
}

void LauncherSocketHandler::handleSocketError()
{
    if (m_socket->error() != QLocalSocket::PeerClosedError) {
        logError(QStringLiteral("Socket error: %1").arg(m_socket->errorString()));
        m_socket->disconnect();
        qApp->quit();
    }
}

void LauncherSocketHandler::handleSocketClosed()
{
    logWarn("The connection has closed unexpectedly, shutting down");
    m_socket->disconnect();
    qApp->quit();
}

void LauncherSocketHandler::handleProcessError(Process *process)
{
    // In case of FailedToStart we won't receive finished signal, so we send the error
    // packet and remove the process here and now. For all other errors we should expect
    // corresponding finished signal to appear, so we will send the error data together with
    // the finished packet later on.
    if (process->error() == QProcess::FailedToStart)
        handleProcessFinished(process);
}

void LauncherSocketHandler::handleProcessStarted(Process *process)
{
    ProcessStartedPacket packet(process->token());
    packet.processId = process->processId();
    process->processStartHandler()->handleProcessStarted();
    sendPacket(packet);
}

void LauncherSocketHandler::handleReadyReadStandardOutput(Process *process)
{
    ReadyReadStandardOutputPacket packet(process->token());
    packet.standardChannel = process->readAllStandardOutput();
    sendPacket(packet);
}

void LauncherSocketHandler::handleReadyReadStandardError(Process *process)
{
    ReadyReadStandardErrorPacket packet(process->token());
    packet.standardChannel = process->readAllStandardError();
    sendPacket(packet);
}

void LauncherSocketHandler::handleProcessFinished(Process *process)
{
    ProcessDonePacket packet(process->token());
    packet.exitCode = process->exitCode();
    packet.exitStatus = process->exitStatus();
    packet.error = process->error();
    packet.errorString = process->errorString();
    packet.stdErr = process->readAllStandardError();
    packet.stdOut = process->readAllStandardOutput();
    sendPacket(packet);
    removeProcess(process->token());
}

void LauncherSocketHandler::handleStartPacket()
{
    Process *& process = m_processes[m_packetParser.token()];
    if (!process)
        process = setupProcess(m_packetParser.token());
    if (process->state() != QProcess::NotRunning) {
        logWarn("Got start request while process was running");
        return;
    }
    const auto packet = LauncherPacket::extractPacket<StartProcessPacket>(
                m_packetParser.token(),
                m_packetParser.packetData());
    process->setEnvironment(packet.env);
    process->setWorkingDirectory(packet.workingDir);
    // Forwarding is handled by the LauncherInterface
    process->setStandardInputFile(packet.standardInputFile);
    ProcessStartHandler *handler = process->processStartHandler();
    handler->setProcessMode(packet.processMode);
    handler->setWriteData(packet.writeData);
    if (packet.belowNormalPriority)
        handler->setBelowNormalPriority();
    handler->setNativeArguments(packet.nativeArguments);
    if (packet.lowPriority)
        process->setLowPriority();
    if (packet.unixTerminalDisabled)
        process->setUnixTerminalDisabled();
    process->setUseCtrlCStub(packet.useCtrlCStub);
    process->start(packet.command, packet.arguments, handler->openMode());
    handler->handleProcessStart();
}

void LauncherSocketHandler::handleWritePacket()
{
    Process * const process = m_processes.value(m_packetParser.token());
    if (!process) {
        logWarn("Got write request for unknown process");
        return;
    }
    if (process->state() != QProcess::Running) {
        logDebug("Can't write into not running process");
        return;
    }
    const auto packet = LauncherPacket::extractPacket<WritePacket>(
                m_packetParser.token(),
                m_packetParser.packetData());
    process->write(packet.inputData);
}

void LauncherSocketHandler::handleStopPacket()
{
    Process * const process = m_processes.value(m_packetParser.token());
    if (!process) {
        // This can happen when the process finishes on its own at about the same time the client
        // sends the request. In this case the process was already deleted.
        logDebug("Got stop request for unknown process");
        return;
    }
    const auto packet = LauncherPacket::extractPacket<StopProcessPacket>(
                m_packetParser.token(),
                m_packetParser.packetData());

    if (packet.signalType == StopProcessPacket::SignalType::Terminate) {
        process->terminate();
        return;
    }

    if (process->state() == QProcess::NotRunning) {
        // This shouldn't happen, since as soon as process finishes or error occurrs
        // the process is being removed.
        logWarn("Got stop request when process was not running");
    } else {
        // We got the client request to stop the starting / running process.
        // We report process exit to the client.
        ProcessDonePacket packet(process->token());
        packet.error = QProcess::Crashed;
        packet.exitCode = -1;
        packet.exitStatus = QProcess::CrashExit;
        packet.stdErr = process->readAllStandardError();
        packet.stdOut = process->readAllStandardOutput();
        sendPacket(packet);
    }
    removeProcess(process->token());
}

void LauncherSocketHandler::handleShutdownPacket()
{
    logDebug("Got shutdown request, closing down");
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
    connect(p, &QProcess::started, this, [this, p] { handleProcessStarted(p); });
    connect(p, &QProcess::errorOccurred, this, [this, p] { handleProcessError(p); });
    connect(p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this, p] { handleProcessFinished(p); });
    connect(p, &QProcess::readyReadStandardOutput,
            this, [this, p] { handleReadyReadStandardOutput(p); });
    connect(p, &QProcess::readyReadStandardError,
            this, [this, p] { handleReadyReadStandardError(p); });
    return p;
}

void LauncherSocketHandler::removeProcess(quintptr token)
{
    const auto it = m_processes.constFind(token);
    if (it == m_processes.constEnd())
        return;

    Process *process = it.value();
    m_processes.erase(it);
    ProcessReaper::reap(process);
}

} // namespace Internal
} // namespace Utils

#include <launchersockethandler.moc>
