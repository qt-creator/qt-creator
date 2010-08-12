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

#include <coreplugin/ssh/sftpdefs.h>
#include <projectexplorer/buildstep.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>

// #define DEPLOY_VIA_MOUNT

QT_BEGIN_NAMESPACE
class QEventLoop;
#ifdef DEPLOY_VIA_MOUNT
class QTimer;
#endif
QT_END_NAMESPACE

namespace Core {
#ifndef DEPLOY_VIA_MOUNT
class SftpChannel;
#endif
class SshConnection;
class SshRemoteProcess;
}

namespace Qt4ProjectManager {
namespace Internal {
#ifdef DEPLOY_VIA_MOUNT
class MaemoRemoteMounter;
#endif
class MaemoDeployables;
class MaemoDeviceConfigListModel;
class MaemoPackageCreationStep;

class MaemoDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class MaemoDeployStepFactory;
public:
    MaemoDeployStep(ProjectExplorer::BuildStepList *bc);
    virtual ~MaemoDeployStep();
    MaemoDeviceConfig deviceConfig() const;
    MaemoDeviceConfigListModel *deviceConfigModel() const;
    bool currentlyNeedsDeployment(const QString &host,
        const MaemoDeployable &deployable) const;
    void setDeployed(const QString &host, const MaemoDeployable &deployable);
    MaemoDeployables *deployables() const { return m_deployables; }
#ifdef DEPLOY_VIA_MOUNT
    int mountPort() const { return m_mountPort; }
    void setMountPort(int port) { m_mountPort = port; }
#endif

signals:
    void done();
    void error();

private slots:
    void start();
    void stop();
    void handleConnected();
    void handleConnectionFailure();
#ifdef DEPLOY_VIA_MOUNT
    void handleMounted();
    void handleUnmounted();
    void handleMountError(const QString &errorMsg);
    void handleProgressReport(const QString &progressMsg);
    void handleCopyProcessFinished(int exitStatus);
    void handleCleanupTimeout();
#else
    void handleSftpChannelInitialized();
    void handleSftpChannelInitializationFailed(const QString &error);
    void handleSftpJobFinished(Core::SftpJobId job, const QString &error);
    void handleLinkProcessFinished(int exitStatus);
#endif
    void handleInstallationFinished(int exitStatus);
    void handleInstallerOutput(const QByteArray &output);
    void handleInstallerErrorOutput(const QByteArray &output);

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
    void writeOutput(const QString &text, ProjectExplorer::BuildStep::OutputFormat = BuildStep::NormalOutput);
    void addDeployTimesToMap(QVariantMap &map) const;
    void getDeployTimesFromMap(const QVariantMap &map);
    const MaemoPackageCreationStep *packagingStep() const;
#ifdef DEPLOY_VIA_MOUNT
    QString deployMountPoint() const;
    void deployNextFile();
#else
    bool deploy(const MaemoDeployable &deployable);
#endif

#ifndef DEPLOY_VIA_MOUNT
    QString uploadDir() const;
#endif

    static const QLatin1String Id;

    MaemoDeployables * const m_deployables;
    QSharedPointer<Core::SshConnection> m_connection;
#ifdef DEPLOY_VIA_MOUNT
    typedef QPair<MaemoDeployable, QSharedPointer<Core::SshRemoteProcess> > DeployAction;
    QScopedPointer<DeployAction> m_currentDeployAction;
    QList<MaemoDeployable> m_filesToCopy;
    MaemoRemoteMounter *m_mounter;
    QTimer *m_cleanupTimer;
    bool m_canStart;
    int m_mountPort;
#else
    QSharedPointer<Core::SftpChannel> m_uploader;
    typedef QPair<MaemoDeployable, QString> DeployInfo;
    QMap<Core::SftpJobId, DeployInfo> m_uploadsInProgress;
    QMap<QSharedPointer<Core::SshRemoteProcess>, MaemoDeployable> m_linksInProgress;
#endif
    QSharedPointer<Core::SshRemoteProcess> m_installer;

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
