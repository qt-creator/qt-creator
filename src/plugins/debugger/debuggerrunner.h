/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGERRUNNER_H
#define DEBUGGERRUNNER_H

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

    virtual bool canRun(RunConfigurationPtr runConfiguration, const QString &mode);
    virtual QString displayName() const;
    virtual ProjectExplorer::RunControl *run(RunConfigurationPtr runConfiguration,
        const QString &mode);
    virtual QWidget *configurationWidget(RunConfigurationPtr runConfiguration);

private:
    DebuggerManager *m_manager;
};


class DebuggerRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    DebuggerRunControl(DebuggerManager *manager,
        ApplicationRunConfigurationPtr runConfiguration);

    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;

signals:
    void stopRequested();

private slots:
    void debuggingFinished();
    void slotAddToOutputWindowInline(const QString &output);

private:
    DebuggerManager *m_manager;
    bool m_running;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERRUNNER_H
