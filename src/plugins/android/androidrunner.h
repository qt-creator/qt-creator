// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

#include <solutions/tasking/tasktreerunner.h>

namespace Android::Internal {

class AndroidRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit AndroidRunner(ProjectExplorer::RunControl *runControl);

    Utils::ProcessHandle pid() const { return m_pid; }

    void start() override;
    void stop() override;

signals:
    void canceled();

private:
    void remoteStarted(qint64 pid);
    void remoteFinished(const QString &errString);

    Utils::ProcessHandle m_pid;
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

void setupAndroidRunWorker();

} // namespace Android::Internal
