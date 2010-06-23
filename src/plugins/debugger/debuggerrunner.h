/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/runconfiguration.h>

namespace ProjectExplorer {
class Environment;
}


namespace Debugger {

class DebuggerRunControl;
class DebuggerStartParameters;

namespace Internal {
class DebuggerEngine;
}

//DEBUGGER_EXPORT QDebug operator<<(QDebug str, const DebuggerStartParameters &);

class DEBUGGER_EXPORT DebuggerRunControlFactory
    : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    DebuggerRunControlFactory(QObject *parent, DebuggerEngineType enabledEngines);

    // ProjectExplorer::IRunControlFactory
    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
        const QString &mode) const;
    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration
        *runConfiguration, const QString &mode);
    QString displayName() const;
    QWidget *createConfigurationWidget(ProjectExplorer::RunConfiguration
        *runConfiguration);

    // This is used by the "Non-Standard" scenarios, e.g. Attach to Core.
    // FIXME: What to do in case of a 0 runConfiguration?
    DebuggerRunControl *create(const DebuggerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0);

private:
    DebuggerEngineType m_enabledEngines;
};


// This is a job description containing all data "local" to the jobs, including
// the models of the individual debugger views.
class DEBUGGER_EXPORT DebuggerRunControl
    : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    DebuggerRunControl(ProjectExplorer::RunConfiguration *runConfiguration);
    ~DebuggerRunControl();

    // ProjectExplorer::RunControl
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;
    QString displayName() const;

    void createEngine(const DebuggerStartParameters &startParameters);

    void setCustomEnvironment(ProjectExplorer::Environment env);
    void setEnabledEngines(DebuggerEngineType enabledEngines);

    void startFailed();
    void startSuccessful();
    void debuggingFinished();

    DebuggerState state() const;
    Internal::DebuggerEngine *engine();

    void showMessage(const QString &msg, int channel);

    static bool checkDebugConfiguration(int toolChain,
                                 QString *errorMessage,
                                 QString *settingsCategory = 0,
                                 QString *settingsPage = 0);

private slots:
    void handleStarted();
    void handleFinished();

private:
    void init();

    DebuggerEngineType engineForExecutable(const QString &executable);
    DebuggerEngineType engineForMode(DebuggerStartMode mode);

    Internal::DebuggerEngine *m_engine;
    bool m_running;
    DebuggerEngineType m_enabledEngines;
    QString m_errorMessage;
    QString m_settingsIdHint;
};

} // namespace Debugger

#endif // DEBUGGERRUNNER_H
