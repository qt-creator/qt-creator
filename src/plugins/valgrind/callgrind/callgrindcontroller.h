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

#include <projectexplorer/runcontrol.h>

#include <QProcess>

namespace Valgrind {
namespace Callgrind {

class CallgrindController : public QObject
{
    Q_OBJECT
public:
    enum Option {
        Unknown,
        Dump,
        ResetEventCounters,
        Pause,
        UnPause
    };
    Q_ENUM(Option)

    CallgrindController();
    ~CallgrindController() override;

    void run(Option option);

    /**
     * Make data file available locally, triggers @c localParseDataAvailable.
     *
     * If the valgrind process was run remotely, this transparently
     * downloads the data file first and returns a local path.
     */
    void getLocalDataFile();
    void setValgrindPid(qint64 pid);
    void setValgrindRunnable(const ProjectExplorer::Runnable &runnable);
    void setValgrindOutputFile(const Utils::FilePath &output) { m_valgrindOutputFile = output; }

signals:
    void finished(Valgrind::Callgrind::CallgrindController::Option option);
    void localParseDataAvailable(const Utils::FilePath &file);
    void statusMessage(const QString &msg);

private:
    void handleControllerProcessError(QProcess::ProcessError);

    void cleanupTempFile();

    void controllerProcessFinished();

    ProjectExplorer::ApplicationLauncher *m_controllerProcess = nullptr;
    ProjectExplorer::Runnable m_valgrindRunnable;
    qint64 m_pid = 0;

    Option m_lastOption = Unknown;

    // remote callgrind support
    Utils::FilePath m_valgrindOutputFile; // On the device that runs valgrind
    Utils::FilePath m_hostOutputFile; // On the device that runs creator
};

} // namespace Callgrind
} // namespace Valgrind
