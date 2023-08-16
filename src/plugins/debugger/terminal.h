// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCoreApplication>
#include <QSocketNotifier>

#include <projectexplorer/runcontrol.h>

namespace Utils {
class Process;
class ProcessRunData;
}

namespace Debugger {

class DebuggerRunTool;

namespace Internal {

class Terminal : public QObject
{
    Q_OBJECT

public:
    Terminal(QObject *parent = nullptr);

    void setup();
    bool isUsable() const;

    QByteArray slaveDevice() const { return m_slaveName; }

    int write(const QByteArray &msg);
    bool sendInterrupt();

signals:
    void stdOutReady(const QString &);
    void stdErrReady(const QString &);
    void error(const QString &);

private:
    void onSlaveReaderActivated(int fd);

    bool m_isUsable = false;
    int m_masterFd = -1;
    QSocketNotifier *m_masterReader = nullptr;
    QByteArray m_slaveName;
};


class TerminalRunner : public ProjectExplorer::RunWorker
{
public:
    TerminalRunner(ProjectExplorer::RunControl *runControl,
                   const std::function<Utils::ProcessRunData()> &stubRunnable);

    qint64 applicationPid() const { return m_applicationPid; }
    qint64 applicationMainThreadId() const { return m_applicationMainThreadId; }

    void kickoffProcess();
    void interrupt();

private:
    void start() final;
    void stop() final;

    void stubStarted();
    void stubDone();

    Utils::Process *m_stubProc = nullptr;
    std::function<Utils::ProcessRunData()> m_stubRunnable;
    qint64 m_applicationPid = 0;
    qint64 m_applicationMainThreadId = 0;
};

} // namespace Internal
} // namespace Debugger
