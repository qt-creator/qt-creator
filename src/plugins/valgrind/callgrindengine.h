/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "valgrindengine.h"

#include "callgrind/callgrindparsedata.h"
#include "callgrind/callgrindparser.h"

#include <utils/qtcprocess.h>

namespace Valgrind {
namespace Internal {

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
    QStringList toolArguments() const override;
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

    std::unique_ptr<Utils::QtcProcess> m_controllerProcess;
    ProjectExplorer::Runnable m_valgrindRunnable;
    qint64 m_pid = 0;

    Option m_lastOption = Unknown;

    // remote callgrind support
    Utils::FilePath m_valgrindOutputFile; // On the device that runs valgrind
    Utils::FilePath m_hostOutputFile; // On the device that runs creator

    Callgrind::Parser m_parser;
    bool m_paused = false;

    QString m_argumentForToggleCollect;
};

} // namespace Internal
} // namespace Valgrind
