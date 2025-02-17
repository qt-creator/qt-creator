// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "valgrindengine.h"

#include "callgrind/callgrindparsedata.h"
#include "callgrind/callgrindparser.h"

#include <solutions/tasking/tasktreerunner.h>

namespace Valgrind::Internal {

class CallgrindToolRunner : public ValgrindToolRunner
{
    Q_OBJECT

public:
    explicit CallgrindToolRunner(ProjectExplorer::RunControl *runControl);
    ~CallgrindToolRunner() override;

    void start() override;

    void dump();
    void reset();
    void pause();
    void unpause();

    /// marks the callgrind process as paused
    /// calls pause() and unpause() if there's an active run
    void setPaused(bool paused);

    void setToggleCollectFunction(const QString &toggleCollectFunction);

protected:
    void addToolArguments(Utils::CommandLine &cmd) const override;

signals:
    void parserDataReady(const Callgrind::ParseDataPtr &data);

private:
    Tasking::ExecutableItem parseRecipe();
    void executeController(const Tasking::Group &recipe);

    Tasking::TaskTreeRunner m_controllerRunner;
    qint64 m_pid = 0;
    bool m_markAsPaused = false;
    Utils::FilePath m_remoteOutputFile; // On the device that runs valgrind
    QString m_argumentForToggleCollect;
};

} // namespace Valgrind::Internal
