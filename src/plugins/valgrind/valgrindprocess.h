// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <solutions/tasking/tasktree.h>

#include <utils/outputformat.h>

#include <QProcess>

QT_BEGIN_NAMESPACE
class QHostAddress;
QT_END_NAMESPACE

namespace Utils {
class CommandLine;
class ProcessRunData;
}

namespace Valgrind {

namespace XmlProtocol {
class Error;
class Status;
}

class ValgrindProcessPrivate;

class ValgrindProcess : public QObject
{
    Q_OBJECT

public:
    explicit ValgrindProcess(QObject *parent = nullptr);
    ~ValgrindProcess() override;

    void setValgrindCommand(const Utils::CommandLine &command);
    void setDebuggee(const Utils::ProcessRunData &debuggee);
    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    void setLocalServerAddress(const QHostAddress &localServerAddress);
    void setUseTerminal(bool on);

    bool start();
    void stop();
    bool runBlocking();

signals:
    void appendMessage(const QString &, Utils::OutputFormat);
    void logMessageReceived(const QByteArray &);
    void processErrorReceived(const QString &, QProcess::ProcessError);
    void valgrindStarted(qint64 pid);
    void done(bool success);

    // Parser's signals
    void status(const Valgrind::XmlProtocol::Status &status);
    void error(const Valgrind::XmlProtocol::Error &error);
    void internalError(const QString &errorString);

private:
    std::unique_ptr<ValgrindProcessPrivate> d;
};

class ValgrindProcessTaskAdapter : public Tasking::TaskAdapter<ValgrindProcess>
{
public:
    ValgrindProcessTaskAdapter() {
        connect(task(), &ValgrindProcess::done, this, &Tasking::TaskInterface::done);
    }
    void start() final { task()->start(); }
};

using ValgrindProcessTask = Tasking::CustomTask<ValgrindProcessTaskAdapter>;

} // namespace Valgrind
