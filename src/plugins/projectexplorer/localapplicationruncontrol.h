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

#ifndef LOCALAPPLICATIONRUNCONTROL_H
#define LOCALAPPLICATIONRUNCONTROL_H

#include "runconfiguration.h"
#include "applicationlauncher.h"

namespace ProjectExplorer {
namespace Internal {

class LocalApplicationRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    LocalApplicationRunControlFactory ();
    ~LocalApplicationRunControlFactory();
    bool canRun(RunConfiguration *runConfiguration, Core::Id mode) const;
    RunControl* create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage);
};

} // namespace Internal

class PROJECTEXPLORER_EXPORT LocalApplicationRunControl : public RunControl
{
    Q_OBJECT
public:
    LocalApplicationRunControl(RunConfiguration *runConfiguration, Core::Id mode);
    ~LocalApplicationRunControl();
    void start();
    StopResult stop();
    bool isRunning() const;

    void setCommand(const QString &executable, const QString &commandLineArguments);
    void setApplicationLauncherMode(const ApplicationLauncher::Mode mode);
    void setWorkingDirectory(const QString &workingDirectory);

private slots:
    void processStarted();
    void processExited(int exitCode, QProcess::ExitStatus status);
    void slotAppendMessage(const QString &err, Utils::OutputFormat isError);
private:
    ApplicationLauncher m_applicationLauncher;
    QString m_executable;
    QString m_commandLineArguments;
    ApplicationLauncher::Mode m_runMode;
    bool m_running;
};

} // namespace ProjectExplorer

#endif // LOCALAPPLICATIONRUNCONTROL_H
