// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dapclient.h"

#include <utils/commandline.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <QLocalSocket>
#include <QTcpSocket>

namespace Debugger::Internal {

// An adapter run as a child process, spoken to over its standard streams.
class ProcessDataProvider final : public IDataProvider
{
    Q_OBJECT

public:
    ProcessDataProvider(const Utils::ProcessRunData &runData,
                        const Utils::CommandLine &cmd,
                        QObject *parent = nullptr);
    ~ProcessDataProvider() final;

    Utils::ProcessResultData resultData() const;

private:
    void start() final;
    bool isRunning() const final;
    void writeRaw(const QByteArray &data) final;
    void kill() final;
    void interrupt() final;
    QByteArray readAllStandardOutput() final;
    QString readAllStandardError() final;
    int exitCode() const final;
    QString executable() const final;
    QProcess::ExitStatus exitStatus() const final;
    QProcess::ProcessError error() const final;
    Utils::ProcessResult result() const final;
    QString exitMessage() const final;

    Utils::Process m_process;
    const Utils::ProcessRunData m_runData;
    const Utils::CommandLine m_cmd;
};

// An adapter already listening on a local socket or a named pipe.
class LocalSocketDataProvider final : public IDataProvider
{
    Q_OBJECT

public:
    LocalSocketDataProvider(const QString &socketName, QObject *parent = nullptr);
    ~LocalSocketDataProvider() final;

private:
    void start() final;
    bool isRunning() const final;
    void writeRaw(const QByteArray &data) final;
    void kill() final;
    QByteArray readAllStandardOutput() final;
    QString readAllStandardError() final;
    int exitCode() const final;
    QString executable() const final;
    QProcess::ExitStatus exitStatus() const final;
    QProcess::ProcessError error() const final;
    Utils::ProcessResult result() const final;
    QString exitMessage() const final;
    void reportDone();

    QLocalSocket m_socket;
    const QString m_socketName;
    // What the socket last failed with, until it has been read out, and that
    // it failed at all: a socket that never connected is still an open
    // QIODevice, so isOpen() does not say.
    QString m_error;
    bool m_failed = false;
    bool m_done = false;
};

// An adapter already listening on a TCP port. Nothing is started here: whoever
// named the port is responsible for there being something behind it.
class TcpDataProvider final : public IDataProvider
{
    Q_OBJECT

public:
    TcpDataProvider(const QString &host, quint16 port, QObject *parent = nullptr);
    ~TcpDataProvider() final;

private:
    void start() final;
    bool isRunning() const final;
    void writeRaw(const QByteArray &data) final;
    void kill() final;
    QByteArray readAllStandardOutput() final;
    QString readAllStandardError() final;
    int exitCode() const final;
    QString executable() const final;
    QProcess::ExitStatus exitStatus() const final;
    QProcess::ProcessError error() const final;
    Utils::ProcessResult result() const final;
    QString exitMessage() const final;
    void reportDone();

    QTcpSocket m_socket;
    const QString m_host;
    const quint16 m_port;
    QString m_error;
    bool m_failed = false;
    bool m_done = false;
};

} // namespace Debugger::Internal
