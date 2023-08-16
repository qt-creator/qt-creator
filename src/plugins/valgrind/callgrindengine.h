// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "valgrindengine.h"

#include "callgrind/callgrindparsedata.h"
#include "callgrind/callgrindparser.h"

#include <utils/process.h>
#include <utils/processinterface.h>

namespace Valgrind::Internal {

class CallgrindToolRunner : public ValgrindToolRunner
{
    Q_OBJECT

public:
    explicit CallgrindToolRunner(ProjectExplorer::RunControl *runControl);
    ~CallgrindToolRunner() override;

    void start() override;

    Valgrind::Callgrind::ParseData *takeParserData();

    /// controller actions
    void dump() { run(Dump); }
    void reset() { run(ResetEventCounters); }
    void pause() { run(Pause); }
    void unpause() { run(UnPause); }

    /// marks the callgrind process as paused
    /// calls pause() and unpause() if there's an active run
    void setPaused(bool paused);

    void setToggleCollectFunction(const QString &toggleCollectFunction);

    enum Option {
        Unknown,
        Dump,
        ResetEventCounters,
        Pause,
        UnPause
    };

    Q_ENUM(Option)

protected:
    void addToolArguments(Utils::CommandLine &cmd) const override;
    QString progressTitle() const override;

signals:
    void parserDataReady(CallgrindToolRunner *engine);

private:
    void showStatusMessage(const QString &message);

    /**
     * Make data file available locally, triggers @c localParseDataAvailable.
     *
     * If the valgrind process was run remotely, this transparently
     * downloads the data file first and returns a local path.
     */
    void triggerParse();
    void controllerFinished(Option option);

    void run(Option option);

    void cleanupTempFile();
    void controllerProcessDone();

    bool m_markAsPaused = false;

    std::unique_ptr<Utils::Process> m_controllerProcess;
    Utils::ProcessRunData m_valgrindRunnable;
    qint64 m_pid = 0;

    Option m_lastOption = Unknown;

    // remote callgrind support
    Utils::FilePath m_valgrindOutputFile; // On the device that runs valgrind
    Utils::FilePath m_hostOutputFile; // On the device that runs creator

    Callgrind::Parser m_parser;
    bool m_paused = false;

    QString m_argumentForToggleCollect;
};

} // namespace Valgrind::Internal
