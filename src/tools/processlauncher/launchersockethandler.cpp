// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "launchersockethandler.h"
#include "launcherlogging.h"

#include <utils/processreaper.h>
#include <utils/processutils.h>

#include <QCoreApplication>
#include <QLocalSocket>
#include <QProcess>

namespace Utils {
namespace Internal {

class ProcessWithToken : public ProcessHelper
{
    Q_OBJECT
public:
    ProcessWithToken(quintptr token, QObject *parent = nullptr) :
        ProcessHelper(parent), m_token(token) { }

    quintptr token() const { return m_token; }
    void setReaperTimeout(int msecs) { m_reaperTimeout = msecs; };
    int reaperTimeout() const { return m_reaperTimeout; }

private:
    const quintptr m_token;
    int m_reaperTimeout = 500;
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
        ProcessWithToken *p = it.value();
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
    case LauncherPacketType::ControlProcess:
        handleControlPacket();
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

void LauncherSocketHandler::handleProcessError(ProcessWithToken *process)
{
    // In case of FailedToStart we won't receive finished signal, so we send the error
    // packet and remove the process here and now. For all other errors we should expect
    // corresponding finished signal to appear, so we will send the error data together with
    // the finished packet later on.
    if (process->error() == QProcess::FailedToStart)
        handleProcessFinished(process);
}

void LauncherSocketHandler::handleProcessStarted(ProcessWithToken *process)
{
    ProcessStartedPacket packet(process->token());
    packet.processId = process->processId();
    process->processStartHandler()->handleProcessStarted();
    sendPacket(packet);
}

void LauncherSocketHandler::handleReadyReadStandardOutput(ProcessWithToken *process)
{
    ReadyReadStandardOutputPacket packet(process->token());
    packet.standardChannel = process->readAllStandardOutput();
    sendPacket(packet);
}

void LauncherSocketHandler::handleReadyReadStandardError(ProcessWithToken *process)
{
    ReadyReadStandardErrorPacket packet(process->token());
    packet.standardChannel = process->readAllStandardError();
    sendPacket(packet);
}

void LauncherSocketHandler::handleProcessFinished(ProcessWithToken *process)
{
    ProcessDonePacket packet(process->token());
    packet.exitCode = process->exitCode();
    packet.exitStatus = process->exitStatus();
    packet.error = process->error();
    packet.errorString = process->errorString();
    if (process->processChannelMode() != QProcess::MergedChannels)
        packet.stdErr = process->readAllStandardError();
    packet.stdOut = process->readAllStandardOutput();
    sendPacket(packet);
    removeProcess(process->token());
}

void LauncherSocketHandler::handleStartPacket()
{
    ProcessWithToken *& process = m_processes[m_packetParser.token()];
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
    process->setProcessChannelMode(packet.processChannelMode == QProcess::MergedChannels
                                   ? QProcess::MergedChannels : QProcess::SeparateChannels);
    process->setStandardInputFile(packet.standardInputFile);
    ProcessStartHandler *handler = process->processStartHandler();
    handler->setWindowsSpecificStartupFlags(packet.belowNormalPriority,
                                            packet.createConsoleOnWindows);
    handler->setProcessMode(packet.processMode);
    handler->setWriteData(packet.writeData);
    handler->setNativeArguments(packet.nativeArguments);
    if (packet.lowPriority)
        process->setLowPriority();
    if (packet.unixTerminalDisabled)
        process->setUnixTerminalDisabled();
    process->setUseCtrlCStub(packet.useCtrlCStub);
    process->setReaperTimeout(packet.reaperTimeout);
    process->start(packet.command, packet.arguments, handler->openMode());
    handler->handleProcessStart();
}

void LauncherSocketHandler::handleWritePacket()
{
    ProcessWithToken * const process = m_processes.value(m_packetParser.token());
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

void LauncherSocketHandler::handleControlPacket()
{
    ProcessWithToken * const process = m_processes.value(m_packetParser.token());
    if (!process) {
        // This can happen when the process finishes on its own at about the same time the client
        // sends the request. In this case the process was already deleted.
        logDebug("Got stop request for unknown process");
        return;
    }
    const auto packet = LauncherPacket::extractPacket<ControlProcessPacket>(
                m_packetParser.token(),
                m_packetParser.packetData());

    switch (packet.signalType) {
    case ControlProcessPacket::SignalType::Terminate:
        process->terminate();
        break;
    case ControlProcessPacket::SignalType::Kill:
        process->kill();
        break;
    case ControlProcessPacket::SignalType::Close:
        removeProcess(process->token());
        break;
    case ControlProcessPacket::SignalType::CloseWriteChannel:
        process->closeWriteChannel();
        break;
    }
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

ProcessWithToken *LauncherSocketHandler::setupProcess(quintptr token)
{
    const auto p = new ProcessWithToken(token, this);
    connect(p, &QProcess::started, this, [this, p] { handleProcessStarted(p); });
    connect(p, &QProcess::errorOccurred, this, [this, p] { handleProcessError(p); });
    connect(p, &QProcess::finished, this, [this, p] { handleProcessFinished(p); });
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

    ProcessWithToken *process = it.value();
    m_processes.erase(it);
    ProcessReaper::reap(process, process->reaperTimeout());
}

} // namespace Internal
} // namespace Utils

#include "launchersockethandler.moc"
