/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGERRUNNER_H
#define DEBUGGERRUNNER_H

#include "debuggermanager.h"

#include <projectexplorer/runconfiguration.h>

namespace ProjectExplorer {
class ApplicationRunConfiguration;
}

namespace Debugger {
namespace Internal {

class DebuggerManager;
class StartData;

typedef QSharedPointer<ProjectExplorer::RunConfiguration>
    RunConfigurationPtr;

typedef QSharedPointer<ProjectExplorer::ApplicationRunConfiguration>
    ApplicationRunConfigurationPtr;

class DebuggerRunner : public ProjectExplorer::IRunConfigurationRunner
{
    Q_OBJECT

public:
    explicit DebuggerRunner(DebuggerManager *manager);

    // ProjectExplorer::IRunConfigurationRunner
    virtual bool canRun(RunConfigurationPtr runConfiguration, const QString &mode);
    virtual ProjectExplorer::RunControl *run(RunConfigurationPtr runConfiguration, const QString &mode);
    virtual QString displayName() const;

    virtual QWidget *configurationWidget(RunConfigurationPtr runConfiguration);

    virtual ProjectExplorer::RunControl
            *run(RunConfigurationPtr runConfiguration,
                 const QString &mode,
                 const QSharedPointer<DebuggerStartParameters> &sp,
                 DebuggerStartMode startMode);

    static RunConfigurationPtr createDefaultRunConfiguration(const QString &executable = QString());

private:
    QSharedPointer<DebuggerStartParameters> m_startParameters;
    DebuggerManager *m_manager;
};

// This is a job description
class DebuggerRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    explicit DebuggerRunControl(DebuggerManager *manager,
                                DebuggerStartMode mode,
                                const QSharedPointer<DebuggerStartParameters> &sp,
                                ApplicationRunConfigurationPtr runConfiguration);

    DebuggerStartMode startMode() const { return m_mode; }

    // ProjectExplorer::RunControl
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;

    Q_SLOT void debuggingFinished();

signals:
    void stopRequested();

private slots:
    void slotAddToOutputWindowInline(const QString &output);

private:
    const DebuggerStartMode m_mode;
    const QSharedPointer<DebuggerStartParameters> m_startParameters;
    DebuggerManager *m_manager;
    bool m_running;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERRUNNER_H
