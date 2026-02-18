// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpstdiotransport.h"

#include <utils/qtcprocess.h>

#include <QLoggingCategory>

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

    m_stderrBuffer.clear();

    FilePath executable = m_cmd.executable();
    if (executable.isEmpty()) {
        emit errorOccurred(tr("No command configured for ACP server."));
        emit finished();
        return;
    }
    if (!executable.isExecutableFile()) {
        executable = executable.searchInPath();
        if (!executable.isExecutableFile()) {
            // Report the originally configured path in the error message, which is more likely to
            // be helpful to the user than the searched path
            executable = m_cmd.executable();
            const QString errorMessage
                = executable.isAbsolutePath()
                ? tr("Command not found: \"%1\". Check that it exists and is executable.")
                      : tr("Command not found: \"%1\". Check that it is executable and on your "
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
    connect(m_process, &Process::started, this, &AcpTransport::started);
    connect(m_process, &Process::done, this, [this] {
        if (!m_expectStop && m_process->result() != ProcessResult::FinishedWithSuccess) {
            if (m_cmd.executable().baseName() == "npx" && m_process->exitCode() == -4058) {
                // this happens regularly on windows and can be avoided by passing --yes
                // argument to the executable
                m_cmd.setArguments("--yes " + m_cmd.arguments());
                start();
                return;
            }
            qCWarning(logStdio) << "Process finished with error:" << m_cmd.toUserOutput();
            QString msg = m_process->exitMessage();
            const QString stderrText = m_stderrBuffer.trimmed();
            if (!stderrText.isEmpty())
                msg += QStringLiteral("\n\n%1").arg(stderrText);
            emit errorOccurred(msg);
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
}

void AcpStdioTransport::sendData(const QByteArray &data)
{
    if (!m_process) {
        emit errorOccurred(tr("Cannot send data: process has not been started (%1).")
                               .arg(m_cmd.toUserOutput()));
        return;
    }
    if (m_process->state() != QProcess::Running) {
        QString msg = tr("Cannot send data: process \"%1\" is not running (exit code %2).")
                          .arg(m_cmd.toUserOutput())
                          .arg(m_process->exitCode());
        const QString stderrText = m_stderrBuffer.trimmed();
        if (!stderrText.isEmpty())
            msg += QStringLiteral("\n\n%1").arg(stderrText);
        emit errorOccurred(msg);
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
    const QString text = QString::fromUtf8(data);
    qCDebug(logStdio) << "stderr:" << data;

    // Keep a bounded buffer of recent stderr for inclusion in error messages
    m_stderrBuffer += text;
    static constexpr int kMaxStderrBuffer = 4096;
    if (m_stderrBuffer.size() > kMaxStderrBuffer)
        m_stderrBuffer = m_stderrBuffer.right(kMaxStderrBuffer);
}

} // namespace AcpClient::Internal
