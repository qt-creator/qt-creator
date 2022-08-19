// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

#include <QDateTime>

namespace Utils { class QtcProcess; }

namespace Qnx::Internal {

class Slog2InfoRunner : public ProjectExplorer::RunWorker
{
public:
    explicit Slog2InfoRunner(ProjectExplorer::RunControl *runControl);

    void start() override;
    void stop() override;

    bool commandFound() const;

private:
    void handleTestProcessCompleted();
    void launchSlog2Info();

    void readLogStandardOutput();
    void readLogStandardError();
    void handleLogDone();

    void printMissingWarning();
    void readLaunchTime();
    void processLog(bool force);
    void processLogLine(const QString &line);

    QString m_applicationId;

    QDateTime m_launchDateTime;
    bool m_found = false;
    bool m_currentLogs = false;
    QString m_remainingData;

    Utils::QtcProcess *m_launchDateTimeProcess = nullptr;
    Utils::QtcProcess *m_testProcess = nullptr;
    Utils::QtcProcess *m_logProcess = nullptr;
};

} // Qnx::Internal
