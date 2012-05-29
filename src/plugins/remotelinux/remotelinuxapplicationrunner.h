/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef REMOTELINUXAPPLICATIONRUNNER_H
#define REMOTELINUXAPPLICATIONRUNNER_H

#include "remotelinux_export.h"

#include <QObject>
#include <QSharedPointer>

namespace QSsh { class SshConnection; }
namespace Utils { class PortList; }

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class RemoteLinuxRunConfiguration;
class RemoteLinuxUsedPortsGatherer;

namespace Internal {
class AbstractRemoteLinuxApplicationRunnerPrivate;
}

class REMOTELINUX_EXPORT AbstractRemoteLinuxApplicationRunner : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxApplicationRunner)
public:
    AbstractRemoteLinuxApplicationRunner(RemoteLinuxRunConfiguration *runConfig,
        QObject *parent = 0);
    ~AbstractRemoteLinuxApplicationRunner();

    void start();
    void stop();

    void startExecution(const QByteArray &remoteCall);

    QSharedPointer<const LinuxDeviceConfiguration> devConfig() const;
    QSsh::SshConnection *connection() const;
    const RemoteLinuxUsedPortsGatherer *usedPortsGatherer() const;
    Utils::PortList *freePorts();
    QString remoteExecutable() const;
    QString arguments() const;
    QString commandPrefix() const;

    static const qint64 InvalidExitCode;

signals:
    void error(const QString &error);
    void readyForExecution();
    void remoteOutput(const QByteArray &output);
    void remoteErrorOutput(const QByteArray &output);
    void reportProgress(const QString &progressOutput);
    void remoteProcessStarted();
    void remoteProcessFinished(qint64 exitCode);

protected:
    // Override to to additional checks.
    virtual bool canRun(QString &whyNot) const;

    void setDeviceConfiguration(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfig);

    void handleDeviceSetupDone(bool success);
    void handleInitialCleanupDone(bool success);
    void handleInitializationsDone(bool success);
    void handlePostRunCleanupDone();

private slots:
    void handleConnected();
    void handleConnectionFailure();
    void handleCleanupFinished(int exitStatus);
    void handleRemoteProcessStarted();
    void handleRemoteProcessFinished(int exitStatus);
    void handlePortsGathererError(const QString &errorMsg);
    void handleUsedPortsAvailable();
    void handleRemoteStdout();
    void handleRemoteStderr();

private:

    virtual QString killApplicationCommandLine() const;

    // Implement to do custom setup of the device *before* connecting.
    // Call handleDeviceSetupDone() afterwards.
    virtual void doDeviceSetup() = 0;

    // Implement to do additional pre-run cleanup and call handleInitialCleanupDone().
    virtual void doAdditionalInitialCleanup() = 0;

    // Implement to do additional initializations right before the application is ready.
    // Call handleInitializationsDone() afterwards.
    virtual void doAdditionalInitializations() = 0;

    // Implement to do cleanups after application exit and call handlePostRunCleanupDone();
    virtual void doPostRunCleanup() = 0;

    virtual void doAdditionalConnectionErrorHandling() = 0;

    void setInactive();
    void emitError(const QString &errorMsg, bool force = false);
    void cleanup();

    Internal::AbstractRemoteLinuxApplicationRunnerPrivate * const d;
};


class REMOTELINUX_EXPORT GenericRemoteLinuxApplicationRunner : public AbstractRemoteLinuxApplicationRunner
{
    Q_OBJECT
public:
    GenericRemoteLinuxApplicationRunner(RemoteLinuxRunConfiguration *runConfig,
        QObject *parent = 0);
    ~GenericRemoteLinuxApplicationRunner();

protected:
    void doDeviceSetup();
    void doAdditionalInitialCleanup();
    void doAdditionalInitializations();
    void doPostRunCleanup();
    void doAdditionalConnectionErrorHandling();
};

} // namespace RemoteLinux

#endif // REMOTELINUXAPPLICATIONRUNNER_H
