// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishserverprocess.h"

#include <utils/qtcassert.h>

namespace Squish::Internal {

SquishServerProcess::SquishServerProcess(QObject *parent)
    : SquishProcessBase(parent)
{
    connect(&m_process, &Utils::QtcProcess::readyReadStandardOutput,
            this, &SquishServerProcess::onStandardOutput);
}

void SquishServerProcess::start(const Utils::CommandLine &commandLine,
                                const Utils::Environment &environment)
{
    QTC_ASSERT(m_process.state() == QProcess::NotRunning, return);
    // especially when writing server config we re-use the process fast and start the server
    // several times and may crash as the process may not have been cleanly destructed yet
    m_process.close();

    m_serverPort = -1;
    m_process.setCommand(commandLine);
    m_process.setEnvironment(environment);

    setState(Starting);
    m_process.start();
    if (!m_process.waitForStarted()) {
        setState(StartFailed);
        qWarning() << "squishserver did not start within 30s";
    }
}

void SquishServerProcess::stop()
{
    if (m_process.state() != QProcess::NotRunning && m_serverPort > 0) {
        Utils::QtcProcess serverKiller;
        QStringList args;
        args << "--stop" << "--port" << QString::number(m_serverPort);
        serverKiller.setCommand({m_process.commandLine().executable(), args});
        serverKiller.setEnvironment(m_process.environment());
        serverKiller.start();
        if (!serverKiller.waitForFinished()) {
            qWarning() << "Could not shutdown server within 30s";
            setState(StopFailed);
        }
    } else {
        qWarning() << "either no process running or port < 1?"
                   << m_process.state() << m_serverPort;
        setState(StopFailed);
    }
}

void SquishServerProcess::onStandardOutput()
{
    const QByteArray output = m_process.readAllRawStandardOutput();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        if (trimmed.startsWith("Port:")) {
            if (m_serverPort == -1) {
                bool ok;
                int port = trimmed.mid(6).toInt(&ok);
                if (ok) {
                    m_serverPort = port;
                    setState(Started);
                } else {
                    qWarning() << "could not get port number" << trimmed.mid(6);
                    setState(StartFailed);
                }
            } else {
                qWarning() << "got a Port output - don't know why...";
            }
        }
        emit logOutputReceived(QString("Server: ") + QLatin1String(trimmed));
    }
}

void SquishServerProcess::onErrorOutput()
{
    // output that must be sent to the Runner/Server Log
    const QByteArray output = m_process.readAllRawStandardError();
    const QList<QByteArray> lines = output.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            emit logOutputReceived(QString("Server: ") + QLatin1String(trimmed));
    }
}

void SquishServerProcess::onDone()
{
    m_serverPort = -1;
    setState(Stopped);
}

} // namespace Squish::Internal
