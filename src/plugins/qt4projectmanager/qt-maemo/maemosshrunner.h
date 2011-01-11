/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MAEMOSSHRUNNER_H
#define MAEMOSSHRUNNER_H

#include "maemodeviceconfigurations.h"
#include "maemomountspecification.h"

#include <utils/environment.h>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>

namespace Core {
    class SshConnection;
    class SshRemoteProcess;
}

namespace Qt4ProjectManager {
namespace Internal {
class MaemoRemoteMounter;
class MaemoRunConfiguration;
class MaemoUsedPortsGatherer;

class MaemoSshRunner : public QObject
{
    Q_OBJECT
public:
    MaemoSshRunner(QObject *parent, MaemoRunConfiguration *runConfig,
        bool debugging);
    ~MaemoSshRunner();

    void start();
    void stop();

    void startExecution(const QByteArray &remoteCall);

    QSharedPointer<Core::SshConnection> connection() const { return m_connection; }

    const MaemoUsedPortsGatherer *usedPortsGatherer() const { return m_portsGatherer; }
    MaemoPortList *freePorts() { return &m_freePorts; }
    MaemoDeviceConfig deviceConfig() const { return m_devConfig; }
    QString remoteExecutable() const { return m_remoteExecutable; }
    QString arguments() const { return m_appArguments; }
    QList<Utils::EnvironmentItem> userEnvChanges() const { return m_userEnvChanges; }

    static const qint64 InvalidExitCode;

signals:
    void error(const QString &error);
    void mountDebugOutput(const QString &output);
    void readyForExecution();
    void remoteOutput(const QByteArray &output);
    void remoteErrorOutput(const QByteArray &output);
    void reportProgress(const QString &progressOutput);
    void remoteProcessStarted();
    void remoteProcessFinished(qint64 exitCode);

private slots:
    void handleConnected();
    void handleConnectionFailure();
    void handleCleanupFinished(int exitStatus);
    void handleRemoteProcessFinished(int exitStatus);
    void handleMounted();
    void handleUnmounted();
    void handleMounterError(const QString &errorMsg);
    void handlePortsGathererError(const QString &errorMsg);
    void handleUsedPortsAvailable();

private:
    enum State { Inactive, Connecting, PreRunCleaning, PostRunCleaning,
        PreMountUnmounting, Mounting, ReadyForExecution,
        ProcessStarting, StopRequested, GatheringPorts
    };

    void setState(State newState);
    void emitError(const QString &errorMsg);

    void cleanup();
    bool isConnectionUsable() const;
    void mount();
    void unmount();

    MaemoRemoteMounter * const m_mounter;
    MaemoUsedPortsGatherer * const m_portsGatherer;
    const MaemoDeviceConfig m_devConfig;
    const QString m_remoteExecutable;
    const QString m_appArguments;
    const QList<Utils::EnvironmentItem> m_userEnvChanges;
    const MaemoPortList m_initialFreePorts;
    QList<MaemoMountSpecification> m_mountSpecs;

    QSharedPointer<Core::SshConnection> m_connection;
    QSharedPointer<Core::SshRemoteProcess> m_runner;
    QSharedPointer<Core::SshRemoteProcess> m_cleaner;
    QStringList m_procsToKill;
    MaemoPortList m_freePorts;

    int m_exitStatus;
    State m_state;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOSSHRUNNER_H
