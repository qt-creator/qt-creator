// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "valgrindsettings.h"

#include <projectexplorer/runcontrol.h>
#include <utils/environment.h>
#include <valgrind/valgrindrunner.h>

#include <QFutureInterface>

namespace Valgrind::Internal {

class ValgrindToolRunner : public ProjectExplorer::RunWorker
{
public:
    explicit ValgrindToolRunner(ProjectExplorer::RunControl *runControl);

    void start() override;
    void stop() override;

protected:
    virtual QString progressTitle() const = 0;
    virtual QStringList toolArguments() const = 0;

    ValgrindProjectSettings m_settings;
    QFutureInterface<void> m_progress;
    ValgrindRunner m_runner;

private:
    void handleProgressCanceled();
    void handleProgressFinished();
    void runnerFinished();

    void receiveProcessError(const QString &message, QProcess::ProcessError error);

    QStringList genericToolArguments() const;

private:
    bool m_isStopping = false;
};

} // Valgrind::Internal
