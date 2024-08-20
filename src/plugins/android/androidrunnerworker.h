// Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <solutions/tasking/tasktreerunner.h>

namespace ProjectExplorer { class RunControl; }
namespace Utils { class Port; }

namespace Android::Internal {

class RunnerStorage;

class AndroidRunnerWorker : public QObject
{
    Q_OBJECT

public:
    AndroidRunnerWorker(ProjectExplorer::RunControl *runControl, const QString &deviceSerialNumber,
                        int apiLevel);
    ~AndroidRunnerWorker() override;

    void asyncStart();
    void asyncStop();

signals:
    void remoteProcessStarted(const Utils::Port &debugServerPort, const QUrl &qmlServer, qint64 pid);
    void remoteProcessFinished(const QString &errString = QString());

    void remoteOutput(const QString &output);
    void remoteErrorOutput(const QString &output);

    void cancel();

private:
    std::unique_ptr<RunnerStorage> m_storage;
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

} // namespace Android::Internal
