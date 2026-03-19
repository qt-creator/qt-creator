// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpclientstdio.h"

#include <coreplugin/icore.h>

#include <QJsonDocument>
#include <QProcess>

namespace QmlDesigner {

McpClientStdio::McpClientStdio(const QString &clientName, const QString &clientVersion, QObject *parent)
    : McpClient(clientName, clientVersion, parent)
{}

McpClientStdio::~McpClientStdio()
{
    stop();
}

bool McpClientStdio::startProcess(
    const QString &command,
    const QStringList &args,
    const QString &workingDir,
    const QProcessEnvironment &env)
{
    if (m_process)
        stop();

    m_process = new QProcess(this);
    m_process->setProcessEnvironment(env);
    if (!workingDir.isEmpty())
        m_process->setWorkingDirectory(workingDir);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &McpClientStdio::onStdoutReady);
    connect(m_process, &QProcess::readyReadStandardError, this, &McpClientStdio::onStderrReady);
    connect(m_process, &QProcess::started, this, &McpClientStdio::onProcessStarted);
    connect(
        m_process,
        qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        this,
        &McpClientStdio::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &McpClientStdio::onProcessError);

    const QString commandFullPath
        = Core::ICore::libexecPath().pathAppended(command).toFSPathString();
    m_process->start(commandFullPath, args, QIODevice::ReadWrite | QIODevice::Unbuffered);
    if (!m_process->waitForStarted(8000)) {
        emit errorOccurred(tr("Failed to start MCP server: %1").arg(m_process->errorString()));
        return false;
    }
    return true;
}

bool McpClientStdio::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void McpClientStdio::stop(int killTimeoutMs)
{
    if (!m_process)
        return;

    if (m_process->state() != QProcess::NotRunning) {
        m_process->closeWriteChannel();
        m_process->terminate();
        if (!m_process->waitForFinished(killTimeoutMs)) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
    }

    m_process->deleteLater();
    m_process = nullptr;
    m_stdoutBuffer.clear();
    resetSession();
}

void McpClientStdio::sendRpcToServer(const QJsonObject &obj)
{
    const QByteArray line = QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
    const qint64 n = m_process->write(line);
    if (n != line.size())
        emit errorOccurred(tr("Failed to write full JSON line to server"));
}

void McpClientStdio::onStdoutReady()
{
    m_stdoutBuffer.append(m_process->readAllStandardOutput());
    int idx = -1;
    while ((idx = m_stdoutBuffer.indexOf('\n')) != -1) {
        const QByteArray line = m_stdoutBuffer.left(idx);
        m_stdoutBuffer.remove(0, idx + 1);
        if (!line.trimmed().isEmpty())
            handleIncomingLine(line);
    }
}

void McpClientStdio::onStderrReady()
{
    const QByteArray data = m_process->readAllStandardError();
    const QStringList lines = QString::fromUtf8(data).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines)
        emit logMessage(line);
}

void McpClientStdio::onProcessStarted()
{
    emit started();
}

void McpClientStdio::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    emit exited(exitCode, status);
}

void McpClientStdio::onProcessError(QProcess::ProcessError e)
{
    emit errorOccurred(tr("Process error: %1").arg(int(e)));
}

} // namespace QmlDesigner
