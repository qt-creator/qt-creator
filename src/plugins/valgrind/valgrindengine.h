// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "valgrindprocess.h"
#include "valgrindsettings.h"

#include <projectexplorer/runcontrol.h>

#include <QFutureInterface>

namespace Valgrind::Internal {

class ValgrindToolRunner : public ProjectExplorer::RunWorker
{
public:
    explicit ValgrindToolRunner(ProjectExplorer::RunControl *runControl);

    void start() override;
    void stop() override;

protected:
    void setProgressTitle(const QString &title) { m_progressTitle = title; }
    virtual void addToolArguments(Utils::CommandLine &cmd) const = 0;

    ValgrindSettings m_settings{false};
    ValgrindProcess m_runner;

private:
    void receiveProcessError(const QString &message, QProcess::ProcessError error);

    QStringList genericToolArguments() const;

private:
    bool m_isStopping = false;
    QString m_progressTitle;
    QFutureInterface<void> m_progress;
};

} // Valgrind::Internal
