// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

#include <solutions/tasking/tasktreerunner.h>

#include <QDateTime>

namespace Qnx::Internal {

class Slog2InfoRunner : public ProjectExplorer::RunWorker
{
public:
    explicit Slog2InfoRunner(ProjectExplorer::RunControl *runControl);

    void start() override;
    void stop() override;

    bool commandFound() const;

private:
    void processRemainingLogData();
    void processLogInput(const QString &input);
    void processLogLine(const QString &line);

    QString m_applicationId;

    QDateTime m_launchDateTime;
    bool m_found = false;
    bool m_currentLogs = false;
    QString m_remainingData;

    Tasking::TaskTreeRunner m_taskTreeRunner;
};

} // Qnx::Internal
