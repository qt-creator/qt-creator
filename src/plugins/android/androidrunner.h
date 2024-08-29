// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidconfigurations.h"

#include <projectexplorer/runcontrol.h>

#include <qmldebug/qmldebugcommandlinearguments.h>
#include <qmldebug/qmloutputparser.h>

#include <solutions/tasking/tasktreerunner.h>

namespace Android::Internal {

class AndroidRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit AndroidRunner(ProjectExplorer::RunControl *runControl);

    Utils::Port debugServerPort() const { return m_debugServerPort; } // GDB or LLDB
    QUrl qmlServer() const { return m_qmlServer; }
    Utils::ProcessHandle pid() const { return m_pid; }

    void start() override;
    void stop() override;

signals:
    void canceled();
    void qmlServerReady(const QUrl &serverUrl);
    void avdDetected();

private:
    void qmlServerPortReady(Utils::Port port);

    void remoteStarted(const Utils::Port &debugServerPort, const QUrl &qmlServer, qint64 pid);
    void remoteFinished(const QString &errString);
    void remoteStdOut(const QString &output);
    void remoteStdErr(const QString &output);

    QPointer<ProjectExplorer::Target> m_target;
    Utils::Port m_debugServerPort;
    QUrl m_qmlServer;
    Utils::ProcessHandle m_pid;
    QmlDebug::QmlOutputParser m_outputParser;
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

} // namespace Android::Internal
