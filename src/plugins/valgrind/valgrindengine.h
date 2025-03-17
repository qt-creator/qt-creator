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
    explicit ValgrindToolRunner(ProjectExplorer::RunControl *runControl,
                                const QString &progressTitle);

    void start() override;
    void stop() override;

protected:
    void setValgrindCommand(const Utils::CommandLine &command) { m_valgrindCommand = command; }

    ValgrindSettings m_settings{false};
    Utils::CommandLine m_valgrindCommand;
    ValgrindProcess m_runner;

private:
    QString m_progressTitle;
    QFutureInterface<void> m_progress;
};

Utils::CommandLine defaultValgrindCommand(ProjectExplorer::RunControl *runControl,
                                          const ValgrindSettings &settings);

} // Valgrind::Internal
