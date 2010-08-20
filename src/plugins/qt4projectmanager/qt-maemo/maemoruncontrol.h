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
#include "maemopackagecontents.h"

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QFutureInterface>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Debugger {
    class DebuggerManager;
    class DebuggerStartParameters;
} // namespace Debugger

namespace Qt4ProjectManager {
namespace Internal {
class MaemoSshDeployer;
class MaemoSshRunner;
class MaemoRunConfiguration;

class AbstractMaemoRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    explicit AbstractMaemoRunControl(ProjectExplorer::RunConfiguration *runConfig);
    virtual ~AbstractMaemoRunControl();

protected:
    virtual bool isRunning() const;
    virtual void start();
    virtual void stop();

    void startDeployment(bool forDebugging);
    void deploy();
    void stopRunning(bool forDebugging);
    void startExecution();
    void handleError(const QString &errString);
    const QString executableOnHost() const;
    const QString executableFileName() const;
    const QString targetCmdLinePrefix() const;
    QString targetCmdLineSuffix() const;
    const QString remoteDir() const;
    QString packageFileName() const;
    QString packageFilePath() const;
    QString executableFilePathOnTarget() const;

private slots:
    virtual void handleRemoteOutput(const QString &output)=0;
    void handleInitialCleanupFinished();
    void handleDeployThreadFinished();
    void handleRunThreadFinished();
    void handleFileCopied();

protected:
    MaemoRunConfiguration *m_runConfig; // TODO this pointer can be invalid
    const MaemoDeviceConfig m_devConfig;

private:
    bool addDeployableIfNeeded(const MaemoDeployable &deployable);

    virtual void startInternal()=0;
    virtual void stopInternal()=0;
    virtual QString remoteCall() const=0;

    void startInitialCleanup();
    void killRemoteProcesses(const QStringList &apps, bool initialCleanup);
    bool isCleaning() const;
    bool isDeploying() const;
    QString remoteSudo() const;
    QString remoteInstallCommand() const;

    QFutureInterface<void> m_progress;
    QScopedPointer<MaemoSshDeployer> m_sshDeployer;
    QScopedPointer<MaemoSshRunner> m_sshRunner;
    QScopedPointer<MaemoSshRunner> m_sshStopper;
    QScopedPointer<MaemoSshRunner> m_initialCleaner;
    bool m_stoppedByUser;

    QList<MaemoDeployable> m_deployables;
    QMap<QString, QString> m_remoteLinks;
    bool m_needsInstall;
};

class MaemoRunControl : public AbstractMaemoRunControl
{
    Q_OBJECT
public:
    explicit MaemoRunControl(ProjectExplorer::RunConfiguration *runConfiguration);
    ~MaemoRunControl();

private slots:
    virtual void handleRemoteOutput(const QString &output);

private:
    virtual void startInternal();
    virtual void stopInternal();
    virtual QString remoteCall() const;
};

class MaemoDebugRunControl : public AbstractMaemoRunControl
{
    Q_OBJECT
public:
    explicit MaemoDebugRunControl(ProjectExplorer::RunConfiguration *runConfiguration);
    ~MaemoDebugRunControl();
    bool isRunning() const;

private slots:
    virtual void handleRemoteOutput(const QString &output);
    void debuggerOutput(const QString &output);
    void debuggingFinished();

private:
    virtual void startInternal();
    virtual void stopInternal();
    virtual QString remoteCall() const;

    QString gdbServerPort() const;
    void startDebugging();

    Debugger::DebuggerManager *m_debuggerManager;
    QSharedPointer<Debugger::DebuggerStartParameters> m_startParams;

    bool m_debuggingStarted;
    QString m_remoteOutput;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONTROL_H
