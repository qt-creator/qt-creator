/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef S60DEVICEDEBUGRUNCONTROL_H
#define S60DEVICEDEBUGRUNCONTROL_H

#include <debugger/debuggerrunner.h>

namespace Qt4ProjectManager {

class S60DeviceRunConfiguration;
class CodaRunControl;

namespace Internal {

class S60DeviceDebugRunControl : public Debugger::DebuggerRunControl
{
    Q_OBJECT

public:
    explicit S60DeviceDebugRunControl(S60DeviceRunConfiguration *runConfiguration,
                                      const Debugger::DebuggerStartParameters &sp,
                                      const QPair<Debugger::DebuggerEngineType, Debugger::DebuggerEngineType> &masterSlaveEngineTypes);
    virtual void start();
    virtual bool promptToStop(bool *optionalPrompt = 0) const;

private slots:
    void remoteSetupRequested();
    void codaConnected();
    void qmlEngineStateChanged(Debugger::DebuggerState state);
    void codaFinished();
    void handleDebuggingFinished();
    void handleMessageFromCoda(ProjectExplorer::RunControl *aCodaRunControl, const QString &msg, Utils::OutputFormat format);

private:
    CodaRunControl *m_codaRunControl;
    enum {
        ENotUsingCodaRunControl = 0,
        EWaitingForCodaConnection,
        ECodaConnected
    } m_codaState;
};

class S60DeviceDebugRunControlFactory : public ProjectExplorer::IRunControlFactory
{
public:
    explicit S60DeviceDebugRunControlFactory(QObject *parent = 0);
    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode) const;

    ProjectExplorer::RunControl* create(ProjectExplorer::RunConfiguration *runConfiguration, ProjectExplorer::RunMode mode);
    QString displayName() const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICEDEBUGRUNCONTROL_H
