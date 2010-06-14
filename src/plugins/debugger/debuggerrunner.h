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

#include <coreplugin/ssh/sshconnection.h>
#include <projectexplorer/runconfiguration.h>

#include <QtCore/QStringList>

namespace ProjectExplorer {
    class Environment;
}

namespace Debugger {

class DebuggerManager;

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    DebuggerStartParameters();
    void clear();

    QString executable;
    QString displayName;
    QString coreFile;
    QStringList processArgs;
    QStringList environment;
    QString workingDirectory;
    qint64 attachPID;
    bool useTerminal;
    QString crashParameter; // for AttachCrashedExternal
    // for remote debugging
    QString remoteChannel;
    QString remoteArchitecture;
    QString symbolFileName;
    QString serverStartScript;
    QString sysRoot;
    QString debuggerCommand;
    int toolChainType;
    QByteArray remoteDumperLib;
    QString qtInstallPath;

    QString dumperLibrary;
    QStringList dumperLibraryLocations;
    Core::SshServerInfo sshserver;
    DebuggerStartMode startMode;
};

//DEBUGGER_EXPORT QDebug operator<<(QDebug str, const DebuggerStartParameters &);

class DEBUGGER_EXPORT DebuggerRunControlFactory
    : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    explicit DebuggerRunControlFactory(DebuggerManager *manager);

    // ProjectExplorer::IRunControlFactory
    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode) const;
    virtual ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                                                const QString &mode);
    virtual QString displayName() const;

    virtual QWidget *createConfigurationWidget(ProjectExplorer::RunConfiguration *runConfiguration);


    // This is used by the "Non-Standard" scenarios, e.g. Attach to Core.
    ProjectExplorer::RunControl *create(const DebuggerStartParameters &sp);

private:
    DebuggerManager *m_manager;
};

// This is a job description
class DEBUGGER_EXPORT DebuggerRunControl
    : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    DebuggerRunControl(DebuggerManager *manager,
                       const DebuggerStartParameters &startParameters);

    void setCustomEnvironment(ProjectExplorer::Environment env);

    // ProjectExplorer::RunControl
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;
    QString displayName() const;

    Q_SLOT void debuggingFinished();

    const DebuggerStartParameters &sp() const { return m_startParameters; }

signals:
    void stopRequested();

public slots:
    void showDebuggerOutput(const QString &msg)
        { showDebuggerOutput(msg, LogDebug); }
    void showApplicationOutput(const QString &output, bool onStdErr);
    void showDebuggerOutput(const QString &output, int channel);
    void showDebuggerInput(const QString &input, int channel);

private slots:
    void slotMessageAvailable(const QString &data, bool isError);

private:
    void init();
    DebuggerStartParameters m_startParameters;
    DebuggerManager *m_manager;
    bool m_running;
};

} // namespace Debugger

#endif // DEBUGGERRUNNER_H
