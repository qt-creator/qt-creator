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
    m_socket->disconnect();
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        logWarn("socket handler destroyed while connection was active");
        m_socket->close();
    }
    for (auto it = m_processes.cbegin(); it != m_processes.cend(); ++it)
        ProcessReaper::reap(it.value());
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
        logWarn(QStringLiteral("Internal protocol error: invalid packet size %1.")
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
    ProcessErrorPacket packet(proc->token());
    packet.error = proc->error();
    packet.errorString = proc->errorString();
    sendPacket(packet);

    // In case of FailedToStart we won't receive finished signal, so we remove the process here.
    // For all other errors we should expect corresponding finished signal to appear.
    if (proc->error() == QProcess::FailedToStart)
        removeProcess(proc->token());
}

void LauncherSocketHandler::handleProcessStarted()
{
    Process *proc = senderProcess();
    ProcessStartedPacket packet(proc->token());
    packet.processId = proc->processId();
    proc->processStartHandler()->handleProcessStarted();
    sendPacket(packet);
}

void LauncherSocketHandler::handleReadyReadStandardOutput()
{
    Process * proc = senderProcess();
    ReadyReadStandardOutputPacket packet(proc->token());
    packet.standardChannel = proc->readAllStandardOutput();
    sendPacket(packet);
}

void LauncherSocketHandler::handleReadyReadStandardError()
{
    Process * proc = senderProcess();
    ReadyReadStandardErrorPacket packet(proc->token());
    packet.standardChannel = proc->readAllStandardError();
    sendPacket(packet);
}

void LauncherSocketHandler::handleProcessFinished()
{
    Process * proc = senderProcess();
    ProcessFinishedPacket packet(proc->token());
    packet.error = proc->error();
    packet.errorString = proc->errorString();
    packet.exitCode = proc->exitCode();
    packet.exitStatus = proc->exitStatus();
    packet.stdErr = proc->readAllStandardError();
    packet.stdOut = proc->readAllStandardOutput();
    sendPacket(packet);
    removeProcess(proc->token());
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
    // Forwarding is handled by the LauncherInterface
    process->setProcessChannelMode(packet.processChannelMode == QProcess::MergedChannels
                                   ? QProcess::MergedChannels : QProcess::SeparateChannels);
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
    process->start(packet.command, packet.arguments, handler->openMode());
    handler->handleProcessStart();
}

void LauncherSocketHandler::handleWritePacket()
{
    Process * const process = m_processes.value(m_packetParser.token());
    if (!process) {
        logWarn("got write request for unknown process");
        return;
    }
    if (process->state() != QProcess::Running) {
        logDebug("can't write into not running process");
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
        logDebug("got stop request for unknown process");
        return;
    }
    if (process->state() == QProcess::NotRunning) {
        // This shouldn't happen, since as soon as process finishes or error occurrs
        // the process is being removed.
        logWarn("got stop request when process was not running");
    } else {
        // We got the client request to stop the starting / running process.
        // We report process exit to the client.
        ProcessFinishedPacket packet(process->token());
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
    connect(p, &QProcess::readyReadStandardOutput,
            this, &LauncherSocketHandler::handleReadyReadStandardOutput);
    connect(p, &QProcess::readyReadStandardError,
            this, &LauncherSocketHandler::handleReadyReadStandardError);
    connect(p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &LauncherSocketHandler::handleProcessFinished);
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

Process *LauncherSocketHandler::senderProcess() const
{
    return static_cast<Process *>(sender());
}

} // namespace Internal
} // namespace Utils

#include <launchersockethandler.moc>
