/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMORUNCONTROL_H
#define MAEMORUNCONTROL_H

#include "maemodeviceconfigurations.h"
#include "maemosshthread.h"

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QFutureInterface>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>

namespace Debugger {
    class DebuggerManager;
    class DebuggerStartParameters;
} // namespace Debugger

namespace Qt4ProjectManager {
namespace Internal {

class MaemoRunConfiguration;

class AbstractMaemoRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    AbstractMaemoRunControl(ProjectExplorer::RunConfiguration *runConfig);
    virtual ~AbstractMaemoRunControl() {}

protected:
    void startDeployment(bool forDebugging);
    void deploy();
    void stopDeployment();
    bool isDeploying() const;
    const QString executableOnHost() const;
    const QString executableOnTarget() const;
    const QString executableFileName() const;
    const QString sshPort() const;
    const QString targetCmdLinePrefix() const;
    const QString remoteDir() const;
    const QStringList options() const;
private slots:
    void deployProcessFinished();
    void handleFileCopied();

protected:
    MaemoRunConfiguration *runConfig; // TODO this pointer can be invalid
    const MaemoDeviceConfig devConfig;

private:
    virtual void handleDeploymentFinished(bool success)=0;

    QFutureInterface<void> m_progress;
    QScopedPointer<MaemoSshDeployer> sshDeployer;

    struct Deployable
    {
        typedef void (MaemoRunConfiguration::*updateFunc)();
        Deployable(const QString &f, const QString &d, updateFunc u)
            : fileName(f), dir(d), updateTimestamp(u) {}
        QString fileName;
        QString dir;
        updateFunc updateTimestamp;
    };
    QList<Deployable> deployables;
};

class MaemoRunControl : public AbstractMaemoRunControl
{
    Q_OBJECT
public:
    MaemoRunControl(ProjectExplorer::RunConfiguration *runConfiguration);
    ~MaemoRunControl();
    void start();
    void stop();
    bool isRunning() const;

private slots:
    void executionFinished();
    void handleRemoteOutput(const QString &output);

private:
    virtual void handleDeploymentFinished(bool success);
    void startExecution();

    QScopedPointer<MaemoSshRunner> sshRunner;
    QScopedPointer<MaemoSshRunner> sshStopper;
    bool stoppedByUser;
};

class MaemoDebugRunControl : public AbstractMaemoRunControl
{
    Q_OBJECT
public:
    MaemoDebugRunControl(ProjectExplorer::RunConfiguration *runConfiguration);
    ~MaemoDebugRunControl();
    void start();
    void stop();
    bool isRunning() const;
    Q_SLOT void debuggingFinished();

signals:
    void stopRequested();

private slots:
    void gdbServerStarted(const QString &output);
    void debuggerOutput(const QString &output);

private:
    virtual void handleDeploymentFinished(bool success);

    QString gdbServerPort() const;
    void startGdbServer();
    void gdbServerStartFailed(const QString &reason);
    void startDebugging();

    QScopedPointer<MaemoSshRunner> sshRunner;
    QScopedPointer<MaemoSshRunner> sshStopper;
    Debugger::DebuggerManager *debuggerManager;
    QSharedPointer<Debugger::DebuggerStartParameters> startParams;
    int inferiorPid;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONTROL_H
