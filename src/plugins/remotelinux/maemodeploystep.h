/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAEMODEPLOYSTEP_H
#define MAEMODEPLOYSTEP_H

#include "maemodeployable.h"
#include "maemodeployables.h"
#include "maemodeviceconfigurations.h"

#include <projectexplorer/buildstep.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QEventLoop;
QT_END_NAMESPACE

namespace Utils {
class SshConnection;
}

namespace RemoteLinux {
class Qt4BuildConfiguration;
namespace Internal {
class AbstractMaemoPackageCreationStep;
class AbstractMaemoPackageInstaller;
class MaemoDeploymentMounter;
class MaemoDeviceConfig;
class MaemoPackageUploader;
class MaemoRemoteCopyFacility;
class MaemoToolChain;
class Qt4MaemoDeployConfiguration;

class MaemoDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class MaemoDeployStepFactory;
public:
    MaemoDeployStep(ProjectExplorer::BuildStepList *bc);

    virtual ~MaemoDeployStep();
    QSharedPointer<const MaemoDeviceConfig> deviceConfig() const { return m_deviceConfig; }
    const AbstractQt4MaemoTarget *maemotarget() const;
    void setDeviceConfig(int i);
    bool currentlyNeedsDeployment(const QString &host,
        const MaemoDeployable &deployable) const;
    void setDeployed(const QString &host, const MaemoDeployable &deployable);
    MaemoPortList freePorts() const;
    Qt4MaemoDeployConfiguration *maemoDeployConfig() const;

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
    void handleUploadFinished(const QString &errorMsg);
    void handleInstallationFinished(const QString &errorMsg);
    void handleFileCopied(const MaemoDeployable &deployable);
    void handleCopyingFinished(const QString &errorMsg);
    void handleRemoteStdout(const QString &output);
    void handleRemoteStderr(const QString &output);
    void handleDeviceConfigurationsUpdated();

private:
    enum State {
        Inactive, StopRequested, Connecting, Uploading, Mounting, Installing,
        Copying, Unmounting
    };

    MaemoDeployStep(ProjectExplorer::BuildStepList *bc,
        MaemoDeployStep *other);
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &map);

    void ctor();
    void raiseError(const QString &error);
    void writeOutput(const QString &text, OutputFormat = MessageOutput);
    void addDeployTimesToMap(QVariantMap &map) const;
    void getDeployTimesFromMap(const QVariantMap &map);
    const AbstractMaemoPackageCreationStep *packagingStep() const;
    QString deployMountPoint() const;
    const MaemoToolChain *toolChain() const;
    QString uploadDir() const;
    void connectToDevice();
    void upload();
    void runPackageInstaller(const QString &packageFilePath);
    void mount();
    void setDeploymentFinished();
    void setState(State newState);
    void setDeviceConfig(MaemoDeviceConfig::Id internalId);
    const Qt4BuildConfiguration *qt4BuildConfiguration() const;
    MaemoPortList freePorts(const QSharedPointer<const MaemoDeviceConfig> &devConfig) const;

    static const QLatin1String Id;

    QSharedPointer<Utils::SshConnection> m_connection;
    QList<MaemoDeployable> m_filesToCopy;
    MaemoDeploymentMounter *m_mounter;
    MaemoPackageUploader *m_uploader;
    AbstractMaemoPackageInstaller *m_installer;
    MaemoRemoteCopyFacility *m_copyFacility;

    bool m_needsInstall;
    typedef QPair<MaemoDeployable, QString> DeployablePerHost;
    QHash<DeployablePerHost, QDateTime> m_lastDeployed;
    QSharedPointer<const MaemoDeviceConfig> m_deviceConfig;
    QSharedPointer<const MaemoDeviceConfig> m_cachedDeviceConfig;
    State m_state;
    bool m_hasError;
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
} // namespace RemoteLinux

#endif // MAEMODEPLOYSTEP_H
