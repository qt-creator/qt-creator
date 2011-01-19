/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAEMODEPLOYSTEP_H
#define MAEMODEPLOYSTEP_H

#include "maemodeployable.h"
#include "maemodeployables.h"
#include "maemodeviceconfigurations.h"
#include "maemomountspecification.h"

#include <coreplugin/ssh/sftpdefs.h>
#include <projectexplorer/buildstep.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QEventLoop;
class QProcess;
class QTimer;
QT_END_NAMESPACE

namespace Core {
class SftpChannel;
class SshConnection;
class SshRemoteProcess;
}

namespace Qt4ProjectManager {
namespace Internal {
class MaemoRemoteMounter;
class MaemoDeviceConfig;
class MaemoPackageCreationStep;
class AbstractMaemoToolChain;
class MaemoUsedPortsGatherer;

class MaemoDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class MaemoDeployStepFactory;
public:
    MaemoDeployStep(ProjectExplorer::BuildStepList *bc);

    virtual ~MaemoDeployStep();
    QSharedPointer<const MaemoDeviceConfig> deviceConfig() const { return m_deviceConfig; }
    void setDeviceConfig(int i);
    bool currentlyNeedsDeployment(const QString &host,
        const MaemoDeployable &deployable) const;
    void setDeployed(const QString &host, const MaemoDeployable &deployable);
    QSharedPointer<MaemoDeployables> deployables() const { return m_deployables; }
    QSharedPointer<Core::SshConnection> sshConnection() const { return m_connection; }

    bool isDeployToSysrootEnabled() const { return m_deployToSysroot; }
    void setDeployToSysrootEnabled(bool deploy) { m_deployToSysroot = deploy; }

    Q_INVOKABLE void stop();

signals:
    void done();
    void error();
    void deviceConfigChanged();

private slots:
    void start();
    void handleConnected();
    void handleConnectionFailure();
    void handleMounted();
    void handleUnmounted();
    void handleMountError(const QString &errorMsg);
    void handleMountDebugOutput(const QString &output);
    void handleProgressReport(const QString &progressMsg);
    void handleCopyProcessFinished(int exitStatus);
    void handleSysrootInstallerFinished();
    void handleSysrootInstallerOutput();
    void handleSysrootInstallerErrorOutput();
    void handleSftpChannelInitialized();
    void handleSftpChannelInitializationFailed(const QString &error);
    void handleSftpJobFinished(Core::SftpJobId job, const QString &error);
    void handleSftpChannelClosed();
    void handleInstallationFinished(int exitStatus);
    void handleDeviceInstallerOutput(const QByteArray &output);
    void handleDeviceInstallerErrorOutput(const QByteArray &output);
    void handlePortsGathererError(const QString &errorMsg);
    void handlePortListReady();
    void handleDeviceConfigurationsUpdated();

private:
    enum State {
        Inactive, StopRequested, InstallingToSysroot, Connecting,
        UnmountingOldDirs, UnmountingCurrentDirs, GatheringPorts, Mounting,
        InstallingToDevice, UnmountingCurrentMounts, CopyingFile,
        InitializingSftp, Uploading
    };

    MaemoDeployStep(ProjectExplorer::BuildStepList *bc,
        MaemoDeployStep *other);
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }
    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &map);

    void ctor();
    void raiseError(const QString &error);
    void writeOutput(const QString &text, OutputFormat = MessageOutput);
    void addDeployTimesToMap(QVariantMap &map) const;
    void getDeployTimesFromMap(const QVariantMap &map);
    const MaemoPackageCreationStep *packagingStep() const;
    QString deployMountPoint() const;
    const AbstractMaemoToolChain *toolChain() const;
    const AbstractQt4MaemoTarget *maemotarget() const;
    void copyNextFileToDevice();
    void installToSysroot();
    QString uploadDir() const;
    void connectToDevice();
    void unmountOldDirs();
    void setupMount();
    void prepareSftpConnection();
    void runDpkg(const QString &packageFilePath);
    void setState(State newState);
    void unmount();
    void setDeviceConfig(MaemoDeviceConfig::Id internalId);

    static const QLatin1String Id;

    QSharedPointer<MaemoDeployables> m_deployables;
    QSharedPointer<Core::SshConnection> m_connection;
    QProcess *m_sysrootInstaller;
    typedef QPair<MaemoDeployable, QSharedPointer<Core::SshRemoteProcess> > DeviceDeployAction;
    QScopedPointer<DeviceDeployAction> m_currentDeviceDeployAction;
    QList<MaemoDeployable> m_filesToCopy;
    MaemoRemoteMounter *m_mounter;
    bool m_deployToSysroot;
    QSharedPointer<Core::SftpChannel> m_uploader;
    QSharedPointer<Core::SshRemoteProcess> m_deviceInstaller;

    bool m_needsInstall;
    typedef QPair<MaemoDeployable, QString> DeployablePerHost;
    QHash<DeployablePerHost, QDateTime> m_lastDeployed;
    QSharedPointer<const MaemoDeviceConfig> m_deviceConfig;
    QSharedPointer<const MaemoDeviceConfig> m_cachedDeviceConfig;
    MaemoUsedPortsGatherer *m_portsGatherer;
    MaemoPortList m_freePorts;
    State m_state;
};

class MaemoDeployEventHandler : public QObject
{
    Q_OBJECT
public:
    MaemoDeployEventHandler(MaemoDeployStep *deployStep,
        QFutureInterface<bool> &future);

private slots:
    void handleDeployingDone();
    void handleDeployingFailed();
    void checkForCanceled();

private:
    MaemoDeployStep * const m_deployStep;
    const QFutureInterface<bool> m_future;
    QEventLoop * const m_eventLoop;
    bool m_error;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEP_H
