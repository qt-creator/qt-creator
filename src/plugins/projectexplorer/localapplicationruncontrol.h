/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LOCALAPPLICATIONRUNCONTROL_H
#define LOCALAPPLICATIONRUNCONTROL_H

#include "runconfiguration.h"
#include "applicationlauncher.h"
#include "projectexplorerconstants.h"

namespace ProjectExplorer {

class LocalApplicationRunConfiguration;
namespace Internal {

class LocalApplicationRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    LocalApplicationRunControlFactory ();
    virtual ~LocalApplicationRunControlFactory();
    virtual bool canRun(RunConfiguration *runConfiguration, RunMode mode) const;
    virtual QString displayName() const;
    virtual RunControl* create(RunConfiguration *runConfiguration, RunMode mode, QString *errorMessage);
};

class LocalApplicationRunControl : public RunControl
{
    Q_OBJECT
public:
    LocalApplicationRunControl(LocalApplicationRunConfiguration *runConfiguration, RunMode mode);
    virtual ~LocalApplicationRunControl();
    virtual void start();
    virtual StopResult stop();
    virtual bool isRunning() const;
    virtual QIcon icon() const;
private slots:
    void processStarted();
    void processExited(int exitCode);
    void slotAppendMessage(const QString &err, Utils::OutputFormat isError);
private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    QString m_executable;
    QString m_commandLineArguments;
    ProjectExplorer::ApplicationLauncher::Mode m_runMode;
    ProcessHandle m_applicationProcessHandle;
    bool m_running;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // LOCALAPPLICATIONRUNCONTROL_H
