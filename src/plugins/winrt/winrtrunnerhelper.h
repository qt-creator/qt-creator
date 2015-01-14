/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#ifndef WINRTRUNNERHELPER_H
#define WINRTRUNNERHELPER_H

#include "winrtdevice.h"

#include <utils/environment.h>
#include <utils/outputformat.h>

#include <QObject>
#include <QProcess>


namespace Utils { class QtcProcess; }
namespace ProjectExplorer { class RunControl; }

namespace WinRt {
namespace Internal {

class WinRtRunConfiguration;

class WinRtRunnerHelper : public QObject
{
    Q_OBJECT
public:
    WinRtRunnerHelper(ProjectExplorer::RunControl *runControl);
    WinRtRunnerHelper(WinRtRunConfiguration *runConfiguration, QString *errormessage);

    void debug(const QString &debuggerExecutable, const QString &debuggerArguments = QString());
    void start();

    void stop();

    bool waitForStarted(int msecs = 10000);
    void setRunControl(ProjectExplorer::RunControl *runControl);

signals:
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void error(QProcess::ProcessError error);

private slots:
    void onProcessReadyReadStdOut();
    void onProcessReadyReadStdErr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError processError);

private:
    enum RunConf { Start, Stop, Debug };
    void startWinRtRunner(const RunConf &conf);
    bool init(WinRtRunConfiguration *runConfiguration, QString *errorMessage);

    ProjectExplorer::RunControl *m_messenger;
    WinRtRunConfiguration *m_runConfiguration;
    WinRtDevice::ConstPtr m_device;
    Utils::Environment m_environment;
    QString m_runnerFilePath;
    QString m_executableFilePath;
    QString m_debuggerExecutable;
    QString m_debuggerArguments;
    QString m_arguments;
    bool m_uninstallAfterStop;
    Utils::QtcProcess *m_process;
};

} // namespace WinRt
} // namespace Internal

#endif // WINRTRUNNERHELPER_H
