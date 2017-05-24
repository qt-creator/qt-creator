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

#include "winrtdevice.h"

#include <utils/environment.h>
#include <utils/outputformat.h>

#include <QObject>
#include <QProcess>


namespace Utils { class QtcProcess; }
namespace ProjectExplorer { class RunWorker; }

namespace WinRt {
namespace Internal {

class WinRtRunnerHelper : public QObject
{
    Q_OBJECT
public:
    WinRtRunnerHelper(ProjectExplorer::RunWorker *runWorker, QString *errorMessage);

    void debug(const QString &debuggerExecutable, const QString &debuggerArguments = QString());
    void start();

    void stop();

    bool waitForStarted(int msecs = 10000);

signals:
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void error(QProcess::ProcessError error);

private:
    enum RunConf { Start, Stop, Debug };

    void onProcessReadyReadStdOut();
    void onProcessReadyReadStdErr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError processError);

    void startWinRtRunner(const RunConf &conf);
    void appendMessage(const QString &message, Utils::OutputFormat format);

    ProjectExplorer::RunWorker *m_worker = nullptr;
    WinRtDevice::ConstPtr m_device;
    Utils::Environment m_environment;
    QString m_runnerFilePath;
    QString m_executableFilePath;
    QString m_debuggerExecutable;
    QString m_debuggerArguments;
    QString m_arguments;
    bool m_uninstallAfterStop = false;
    Utils::QtcProcess *m_process = nullptr;
};

} // namespace WinRt
} // namespace Internal
