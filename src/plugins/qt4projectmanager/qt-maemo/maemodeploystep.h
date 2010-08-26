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

#ifndef MAEMODEPLOYSTEP_H
#define MAEMODEPLOYSTEP_H

#include "maemodeployable.h"
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
class MaemoDeployables;
class MaemoDeviceConfigListModel;
class MaemoPackageCreationStep;
class MaemoToolChain;

class MaemoDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class MaemoDeployStepFactory;
public:
    MaemoDeployStep(ProjectExplorer::BuildStepList *bc);

    virtual ~MaemoDeployStep();
    MaemoDeviceConfig deviceConfig() const;
    MaemoDeviceConfigListModel *deviceConfigModel() const { return m_deviceConfigModel; }
    bool currentlyNeedsDeployment(const QString &host,
        const MaemoDeployable &deployable) const;
    void setDeployed(const QString &host, const MaemoDeployable &deployable);
    MaemoDeployables *deployables() const { return m_deployables; }
    QSharedPointer<Core::SshConnection> sshConnection() const { return m_connection; }

    bool isDeployToSysrootEnabled() const { return m_deployToSysroot; }
    void setDeployToSysrootEnabled(bool deploy) { m_deployToSysroot = deploy; }

signals:
    void done();
    void error();

private slots:
    void start();
    void stop();
    void handleConnected();
    void handleConnectionFailure();
    void handleMounted();
    void handleUnmounted();
    void handleMountError(const QString &errorMsg);
    void handleMountDebugOutput(const QString &output);
    void handleProgressReport(const QString &progressMsg);
    void handleCopyProcessFinished(int exitStatus);
    void handleCleanupTimeout();
    void handleSysrootInstallerFinished();
    void handleSysrootInstallerOutput();
    void handleSysrootInstallerErrorOutput();
    void handleSftpChannelInitialized();
    void handleSftpChannelInitializationFailed(const QString &error);
    void handleSftpJobFinished(Core::SftpJobId job, const QString &error);
    void handleInstallationFinished(int exitStatus);
    void handleDeviceInstallerOutput(const QByteArray &output);
    void handleDeviceInstallerErrorOutput(const QByteArray &output);

private:
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
    const MaemoToolChain *toolChain() const;
    void copyNextFileToDevice();
    bool addMountSpecification(const MaemoMountSpecification &mountSpec);
    void installToSysroot();
    QString uploadDir() const;
    void connectToDevice();
    void unmountOldDirs();
    void setupMount();
    void prepareSftpConnection();
    void runDpkg(const QString &packageFilePath);

    static const QLatin1String Id;

    MaemoDeployables * const m_deployables;
    QSharedPointer<Core::SshConnection> m_connection;
    QProcess *m_sysrootInstaller;
    typedef QPair<MaemoDeployable, QSharedPointer<Core::SshRemoteProcess> > DeviceDeployAction;
    QScopedPointer<DeviceDeployAction> m_currentDeviceDeployAction;
    QList<MaemoDeployable> m_filesToCopy;
    MaemoRemoteMounter *m_mounter;
    QTimer *m_cleanupTimer;
    bool m_canStart;
    bool m_deployToSysroot;
    enum UnmountState { OldDirsUnmount, CurrentDirsUnmount, CurrentMountsUnmount };
    UnmountState m_unmountState;
    QSharedPointer<Core::SftpChannel> m_uploader;
    QSharedPointer<Core::SshRemoteProcess> m_deviceInstaller;

    bool m_stopped;
    bool m_needsInstall;
    bool m_connecting;
    typedef QPair<MaemoDeployable, QString> DeployablePerHost;
    QHash<DeployablePerHost, QDateTime> m_lastDeployed;
    MaemoDeviceConfigListModel *m_deviceConfigModel;
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
    const MaemoDeployStep * const m_deployStep;
    const QFutureInterface<bool> m_future;
    QEventLoop * const m_eventLoop;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEP_H
