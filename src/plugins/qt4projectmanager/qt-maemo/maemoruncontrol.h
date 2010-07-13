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

#include <coreplugin/ssh/sftpdefs.h>
#include <projectexplorer/runconfiguration.h>

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Core {
    class SftpChannel;
    class SshConnection;
    class SshRemoteProcess;
}

namespace Debugger {
    class DebuggerRunControl;
    class DebuggerStartParameters;
} // namespace Debugger

namespace Qt4ProjectManager {
namespace Internal {
class MaemoRunConfiguration;

class AbstractMaemoRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    explicit AbstractMaemoRunControl(ProjectExplorer::RunConfiguration *runConfig, QString mode);
    virtual ~AbstractMaemoRunControl();

protected:
    virtual bool isRunning() const;
    virtual void start();
    virtual void stop();

    void stopRunning(bool forDebugging);
    virtual void startExecution();
    void handleError(const QString &errString);
    const QString executableOnHost() const;
    const QString executableFileName() const;
    const QString targetCmdLinePrefix() const;
    QString targetCmdLineSuffix() const;
    const QString uploadDir() const;
    QString executableFilePathOnTarget() const;

private slots:
    void handleConnected();
    void handleConnectionFailure();
    void handleInitialCleanupFinished(int exitStatus);
    virtual void handleRemoteProcessStarted() {}
    void handleRemoteProcessFinished(int exitStatus);
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);

protected:
    MaemoRunConfiguration *m_runConfig; // TODO this pointer can be invalid
    const MaemoDeviceConfig m_devConfig;
    QSharedPointer<Core::SshConnection> m_connection;
    bool m_stopped;

private:
    virtual void stopInternal()=0;
    virtual QString remoteCall() const=0;

    void killRemoteProcesses(const QStringList &apps, bool initialCleanup);
    void cancelActions();
    template<class SshChannel> void closeSshChannel(SshChannel &channel);
    void startExecutionIfPossible();

    QSharedPointer<Core::SshRemoteProcess> m_runner;
    QSharedPointer<Core::SshRemoteProcess> m_stopper;
    QSharedPointer<Core::SshRemoteProcess> m_initialCleaner;
};

class MaemoRunControl : public AbstractMaemoRunControl
{
    Q_OBJECT
public:
    explicit MaemoRunControl(ProjectExplorer::RunConfiguration *runConfiguration);
    ~MaemoRunControl();

private:
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
    virtual void handleRemoteProcessStarted();
    void debuggerOutput(const QString &output);
    void debuggingFinished();
    void handleSftpChannelInitialized();
    void handleSftpChannelInitializationFailed(const QString &error);
    void handleSftpJobFinished(Core::SftpJobId job, const QString &error);

private:
    virtual void stopInternal();
    virtual void startExecution();
    virtual QString remoteCall() const;

    QString gdbServerPort() const;
    void startDebugging();
    bool isDeploying() const;

    Debugger::DebuggerRunControl *m_debuggerRunControl;
    QSharedPointer<Debugger::DebuggerStartParameters> m_startParams;
    QSharedPointer<Core::SftpChannel> m_uploader;
    Core::SftpJobId m_uploadJob;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONTROL_H
