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
#include <projectexplorer/applicationrunconfiguration.h>

namespace ProjectExplorer {
class LocalApplicationRunConfiguration;
}

namespace Debugger {
namespace Internal {

class DebuggerManager;
class StartData;

typedef QSharedPointer<ProjectExplorer::RunConfiguration>
    RunConfigurationPtr;

typedef QSharedPointer<ProjectExplorer::LocalApplicationRunConfiguration>
    LocalApplicationRunConfigurationPtr;

class DebuggerRunControlFactory
    : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    explicit DebuggerRunControlFactory(DebuggerManager *manager);

    // ProjectExplorer::IRunControlFactory
    bool canRun(const RunConfigurationPtr &runConfiguration, const QString &mode) const;
    virtual ProjectExplorer::RunControl *create(const RunConfigurationPtr &runConfiguration,
                                                const QString &mode);
    virtual QString displayName() const;

    virtual QWidget *configurationWidget(const RunConfigurationPtr &runConfiguration);

    virtual ProjectExplorer::RunControl *create(const RunConfigurationPtr &runConfiguration,
                                                const QString &mode,
                                                const DebuggerStartParametersPtr &sp);

    static RunConfigurationPtr createDefaultRunConfiguration(const QString &executable = QString());

private:
    DebuggerStartParametersPtr m_startParameters;
    DebuggerManager *m_manager;
};

// This is a job description
class DebuggerRunControl
    : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    DebuggerRunControl(DebuggerManager *manager,
                       const DebuggerStartParametersPtr &startParamters,
                       LocalApplicationRunConfigurationPtr runConfiguration);

    // ProjectExplorer::RunControl
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;

    Q_SLOT void debuggingFinished();
    DebuggerStartParametersPtr startParameters() { return m_startParameters; }

signals:
    void stopRequested();

private slots:
    void slotAddToOutputWindowInline(const QString &output);

private:
    DebuggerStartParametersPtr m_startParameters;
    DebuggerManager *m_manager;
    bool m_running;
};

// A default run configuration for external executables or attaching to
// running processes by id.
class DefaultLocalApplicationRunConfiguration
    : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
public:
    explicit DefaultLocalApplicationRunConfiguration(const QString &executable = QString());

    virtual QString executable() const { return m_executable; }
    virtual RunMode runMode() const { return Gui; }
    virtual QString workingDirectory() const { return QString(); }
    virtual QStringList commandLineArguments() const  { return QStringList(); }
    virtual ProjectExplorer::Environment environment() const
        { return ProjectExplorer::Environment(); }
    virtual QString dumperLibrary() const { return QString(); }
    virtual QStringList dumperLibraryLocations() const { return QStringList(); }
    virtual ProjectExplorer::ToolChain::ToolChainType toolChainType() const
        { return ProjectExplorer::ToolChain::UNKNOWN; }
    virtual QWidget *configurationWidget() { return 0; }

private:
    const QString m_executable;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGERRUNNER_H
