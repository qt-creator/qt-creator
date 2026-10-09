// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpstdiotransport.h"

#include "acpclienttr.h"

#include <utils/fileutils.h>
#include <utils/qtcprocess.h>

#include <QLoggingCategory>

#include <algorithm>

static Q_LOGGING_CATEGORY(logStdio, "qtc.acpclient.stdio", QtWarningMsg);

using namespace Utils;

namespace AcpClient::Internal {

AcpStdioTransport::AcpStdioTransport(QObject *parent)
    : AcpTransport(parent)
{}

AcpStdioTransport::~AcpStdioTransport()
{
    stop();
}

void AcpStdioTransport::setCommandLine(const CommandLine &cmd)
{
    m_cmd = cmd;
    if (HostOsInfo::isWindowsHost() && m_cmd.executable().baseName() == "npx"
        && !m_cmd.arguments().contains("--yes")) {
        m_cmd.setArguments("--yes " + m_cmd.arguments());
    }
}

void AcpStdioTransport::setWorkingDirectory(const FilePath &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

void AcpStdioTransport::setEnvironment(const Environment &environment)
{
    m_env = environment;
}

void AcpStdioTransport::start()
{
    if (m_process) {
        if (m_process->isRunning())
            m_process->kill();
        m_process->deleteLater(); // avoid crash if this get's called from a slot handling a process signal
    }

    m_stderrTail.clear();
    m_stderrDecoder.resetState();
    resetProtocolState();

    FilePath executable = m_cmd.executable();
    if (executable.isEmpty()) {
        emit errorOccurred(Tr::tr("No command configured for ACP server."));
        emit finished();
        return;
    }
    if (!executable.isExecutableFile()) {
        executable = executable.searchInPath(FileUtils::usefulExtraSearchPaths());
        if (!executable.isExecutableFile()) {
            // Report the originally configured path in the error message, which is more likely to
            // be helpful to the user than the searched path
            executable = m_cmd.executable();
            const QString errorMessage
                = executable.isAbsolutePath()
                      ? Tr::tr("Command not found: \"%1\". Check that it exists and is executable.")
                      : Tr::tr(
                            "Command not found: \"%1\". Check that it is executable and on your "
                            "PATH.");
            emit errorOccurred(errorMessage.arg(executable.toUserOutput()));
            emit finished();
            return;
        }
        m_cmd.setExecutable(executable);
    }

    m_process = new Process(this);
    m_process->setProcessMode(ProcessMode::Writer);

    connect(m_process, &Process::readyReadStandardOutput, this, &AcpStdioTransport::readOutput);
    connect(m_process, &Process::readyReadStandardError, this, &AcpStdioTransport::readError);
    connect(m_process, &Process::started, this, [this] {
        setAcceptsConsoleInput(true);
        emit started();
    });
    connect(m_process, &Process::done, this, [this] {
        setAcceptsConsoleInput(false);
        // Once the input was closed the connection has been reported as failed
        // already, and a server usually exits on the end of its input.
        const bool reportExit = !m_expectStop && !isInputClosed();
        if (reportExit && m_process->result() != ProcessResult::FinishedWithSuccess) {
            qCWarning(logStdio) << "Process finished with error:" << m_cmd.toUserOutput();
            emit errorOccurred(exitMessageWithStderr(m_process->exitMessage()));
        } else if (reportExit && !hasReceivedMessage()) {
            // A clean exit is still a failure when the server never answered.
            emit errorOccurred(exitMessageWithStderr(
                Tr::tr("The server exited before it answered.")));
        }
        m_expectStop = false;
        emit finished();
    });

    m_process->setCommand(m_cmd);
    if (!m_workingDirectory.isEmpty())
        m_process->setWorkingDirectory(m_workingDirectory);
    if (m_env)
        m_process->setEnvironment(*m_env);
    else
        m_process->setEnvironment(m_cmd.executable().deviceEnvironment());

    qCDebug(logStdio) << "Starting:" << m_cmd.toUserOutput();
    m_process->start();
}

void AcpStdioTransport::stop()
{
    if (m_process && m_process->isRunning()) {
        m_expectStop = true;
        m_process->kill();
        m_process->waitForFinished(QDeadlineTimer(3000));
    }
    delete m_process;
    m_process = nullptr;
    setAcceptsConsoleInput(false);
}

void AcpStdioTransport::sendConsoleInput(const QByteArray &data)
{
    if (m_process && m_process->state() == ProcessState::Running)
        m_process->writeRaw(data);
}

void AcpStdioTransport::endConsoleInput()
{
    if (m_process && m_process->state() == ProcessState::Running)
        m_process->closeWriteChannel();
}

void AcpStdioTransport::sendData(const QByteArray &data)
{
    if (!m_process) {
        emit errorOccurred(
            Tr::tr("Cannot send data: process has not been started (%1).").arg(m_cmd.toUserOutput()));
        return;
    }
    if (m_process->state() != ProcessState::Running) {
        QString msg = Tr::tr("Cannot send data: process \"%1\" is not running (exit code %2).")
                          .arg(m_cmd.toUserOutput())
                          .arg(m_process->exitCode());
        emit errorOccurred(exitMessageWithStderr(msg));
        return;
    }
    m_process->writeRaw(data);
}

void AcpStdioTransport::readOutput()
{
    if (!m_process)
        return;
    parseData(m_process->readAllRawStandardOutput());
}

void AcpStdioTransport::readError()
{
    if (!m_process)
        return;
    const QByteArray data = m_process->readAllRawStandardError();
    const QString text = m_stderrDecoder.decode(data);
    qCDebug(logStdio) << "stderr:" << data;

    emit consoleOutput(data);

    m_stderrTail.append(text);
}

QString AcpStdioTransport::exitMessageWithStderr(const QString &message) const
{
    const QString stderrText = m_stderrTail.text().trimmed();
    if (stderrText.isEmpty())
        return message;
    return QStringLiteral("%1\n\n%2").arg(message, stderrText);
}

void StderrTail::append(const QString &text)
{
    enum { MaxSize = 4096 };

    for (const QChar c : text) {
        if (m_pendingCarriageReturn) {
            m_pendingCarriageReturn = false;
            if (c != '\n')
                m_text.truncate(m_lineStart);
        }
        if (c == '\r') {
            m_pendingCarriageReturn = true;
        } else if (c == '\n') {
            m_text += c;
            m_lineStart = m_text.size();
        } else {
            m_text += c;
        }
    }

    if (m_text.size() > MaxSize) {
        const int dropped = m_text.size() - MaxSize;
        m_text = m_text.right(MaxSize);
        m_lineStart = std::max(0, m_lineStart - dropped);
    }
}

void StderrTail::clear()
{
    m_text.clear();
    m_lineStart = 0;
    m_pendingCarriageReturn = false;
}

} // namespace AcpClient::Internal
