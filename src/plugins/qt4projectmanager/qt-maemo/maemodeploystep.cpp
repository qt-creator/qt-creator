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

#include "maemodeploystep.h"

#include "maemoconstants.h"
#include "maemodeployables.h"
#include "maemodeploystepwidget.h"
#include "maemodeviceconfiglistmodel.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemoremotemounter.h"
#include "maemorunconfiguration.h"
#include "maemotoolchain.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4buildconfiguration.h>

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

using namespace Core;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {
namespace { const int DefaultMountPort = 1050; }

const QLatin1String MaemoDeployStep::Id("Qt4ProjectManager.MaemoDeployStep");

MaemoDeployStep::MaemoDeployStep(ProjectExplorer::BuildStepList *parent)
    : BuildStep(parent, Id), m_deployables(new MaemoDeployables(this))
{
    ctor();
}

MaemoDeployStep::MaemoDeployStep(ProjectExplorer::BuildStepList *parent,
    MaemoDeployStep *other)
    : BuildStep(parent, other), m_deployables(new MaemoDeployables(this)),
      m_lastDeployed(other->m_lastDeployed)
{
    ctor();
}

MaemoDeployStep::~MaemoDeployStep()
{
    delete m_deployables;
}

void MaemoDeployStep::ctor()
{
    setDisplayName(tr("Deploying to Maemo device"));

    m_connecting = false;
    m_needsInstall = false;
    m_stopped = false;
    m_deviceConfigModel = new MaemoDeviceConfigListModel(this);
#ifdef DEPLOY_VIA_MOUNT
    m_canStart = true;
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setSingleShot(true);
    connect(m_cleanupTimer, SIGNAL(timeout()), this,
        SLOT(handleCleanupTimeout()));
    const Qt4BuildConfiguration * const buildConfig
        = qobject_cast<Qt4BuildConfiguration *>(buildConfiguration());
    const MaemoToolChain * const toolchain
        = dynamic_cast<MaemoToolChain *>(buildConfig->toolChain());
    m_mounter = new MaemoRemoteMounter(this, toolchain);
    connect(m_mounter, SIGNAL(mounted()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SLOT(handleProgressReport(QString)));
#endif
}

bool MaemoDeployStep::init()
{
    return true;
}

void MaemoDeployStep::run(QFutureInterface<bool> &fi)
{
    // Move to GUI thread for connection sharing with run control.
    QTimer::singleShot(0, this, SLOT(start()));

    MaemoDeployEventHandler eventHandler(this, fi);
    connect (&eventHandler, SIGNAL(destroyed()), this, SLOT(stop()));
}

BuildStepConfigWidget *MaemoDeployStep::createConfigWidget()
{
    return new MaemoDeployStepWidget(this);
}

QVariantMap MaemoDeployStep::toMap() const
{
    QVariantMap map(BuildStep::toMap());
    addDeployTimesToMap(map);
    map.unite(m_deviceConfigModel->toMap());
    return map;
}

void MaemoDeployStep::addDeployTimesToMap(QVariantMap &map) const
{
    QVariantList hostList;
    QVariantList fileList;
    QVariantList remotePathList;
    QVariantList timeList;
    typedef QHash<DeployablePerHost, QDateTime>::ConstIterator DepIt;
    for (DepIt it = m_lastDeployed.begin(); it != m_lastDeployed.end(); ++it) {
        fileList << it.key().first.localFilePath;
        remotePathList << it.key().first.remoteDir;
        hostList << it.key().second;
        timeList << it.value();
    }
    map.insert(LastDeployedHostsKey, hostList);
    map.insert(LastDeployedFilesKey, fileList);
    map.insert(LastDeployedRemotePathsKey, remotePathList);
    map.insert(LastDeployedTimesKey, timeList);
}

bool MaemoDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    getDeployTimesFromMap(map);
    m_deviceConfigModel->fromMap(map);
    return true;
}

void MaemoDeployStep::getDeployTimesFromMap(const QVariantMap &map)
{
    const QVariantList &hostList = map.value(LastDeployedHostsKey).toList();
    const QVariantList &fileList = map.value(LastDeployedFilesKey).toList();
    const QVariantList &remotePathList
        = map.value(LastDeployedRemotePathsKey).toList();
    const QVariantList &timeList = map.value(LastDeployedTimesKey).toList();
    const int elemCount
        = qMin(qMin(hostList.size(), fileList.size()),
            qMin(remotePathList.size(), timeList.size()));
    for (int i = 0; i < elemCount; ++i) {
        const MaemoDeployable d(fileList.at(i).toString(),
            remotePathList.at(i).toString());
        m_lastDeployed.insert(DeployablePerHost(d, hostList.at(i).toString()),
            timeList.at(i).toDateTime());
    }
}

const MaemoPackageCreationStep *MaemoDeployStep::packagingStep() const
{
    const MaemoPackageCreationStep * const step
        = MaemoGlobal::buildStep<MaemoPackageCreationStep>(target()->activeDeployConfiguration());
    Q_ASSERT_X(step, Q_FUNC_INFO,
        "Impossible: Maemo build configuration without packaging step.");
    return step;
}

void MaemoDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        Constants::TASK_CATEGORY_BUILDSYSTEM));
    emit error();
}

void MaemoDeployStep::writeOutput(const QString &text,
    BuildStep::OutputFormat format)
{
    emit addOutput(text, format);
}

void MaemoDeployStep::stop()
{
#ifdef DEPLOY_VIA_MOUNT
    if (m_stopped)
        return;

    if (m_connecting || m_needsInstall || !m_filesToCopy.isEmpty()
        || (m_installer && m_installer->isRunning())
        || m_currentDeployAction) {
        if (m_connection && m_connection->state() == SshConnection::Connected
            && ((m_installer && m_installer->isRunning())
                || m_currentDeployAction)) {
            const QByteArray programToKill
                = m_currentDeployAction ? "cp" : "dpkg";
            const QByteArray cmdLine = "pkill -x " + programToKill
                + "; sleep 1; pkill -x -9 " + programToKill;
            SshRemoteProcess::Ptr killProc
                = m_connection->createRemoteProcess(cmdLine);
            killProc->start();
        }
        m_stopped = true;
        m_canStart = false;
        m_needsInstall = false;
        m_filesToCopy.clear();
        m_connecting = false;
        m_cleanupTimer->start(5000);
        m_mounter->stop();
    }
#else
    m_stopped = true;
    if (m_installer && m_installer->isRunning()) {
        disconnect(m_installer.data(), 0, this, 0);
    }
    else if (!m_uploadsInProgress.isEmpty() || !m_linksInProgress.isEmpty()) {
        m_uploadsInProgress.clear();
        m_linksInProgress.clear();
        disconnect(m_uploader.data(), 0, this, 0);
        m_uploader->closeChannel();
    }
    if (m_connection)
        disconnect(m_connection.data(), 0, this, 0);
#endif
}

#ifndef DEPLOY_VIA_MOUNT
QString MaemoDeployStep::uploadDir() const
{
    return MaemoGlobal::homeDirOnDevice(m_connection->connectionParameters().uname);
}
#endif

bool MaemoDeployStep::currentlyNeedsDeployment(const QString &host,
    const MaemoDeployable &deployable) const
{
    const QDateTime &lastDeployed
        = m_lastDeployed.value(DeployablePerHost(deployable, host));
    return !lastDeployed.isValid()
        || QFileInfo(deployable.localFilePath).lastModified() > lastDeployed;
}

void MaemoDeployStep::setDeployed(const QString &host,
    const MaemoDeployable &deployable)
{
    m_lastDeployed.insert(DeployablePerHost(deployable, host),
        QDateTime::currentDateTime());
}

MaemoDeviceConfig MaemoDeployStep::deviceConfig() const
{
    return deviceConfigModel()->current();
}

MaemoDeviceConfigListModel *MaemoDeployStep::deviceConfigModel() const
{
    const MaemoRunConfiguration * const rc =
        qobject_cast<const MaemoRunConfiguration *>(buildConfiguration()
            ->target()->activeRunConfiguration());
    return rc ? rc->deviceConfigModel() : m_deviceConfigModel;
}

void MaemoDeployStep::start()
{
#ifdef DEPLOY_VIA_MOUNT
    if (!m_canStart) {
        raiseError(tr("Can't start deployment, haven't cleaned up from last time yet."));
        return;
    }
    m_cleanupTimer->stop();
#endif
    m_stopped = false;

    const MaemoDeviceConfig &devConfig = deviceConfig();
    if (!devConfig.isValid()) {
        raiseError(tr("Deployment failed: No valid device set."));
        return;
    }

    if (m_connection)
        disconnect(m_connection.data(), 0, this, 0);
    const bool canReUse = m_connection
        && m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == devConfig.server;
    if (!canReUse)
        m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(SshError)), this,
        SLOT(handleConnectionFailure()));
    if (canReUse) {
        handleConnected();
    } else {
        writeOutput(tr("Connecting to device..."));
        m_connecting = true;
        m_connection->connectToHost(devConfig.server);
    }
}

void MaemoDeployStep::handleConnected()
{
    m_connecting = false;
    if (m_stopped) {
#ifdef DEPLOY_VIA_MOUNT
        m_canStart = true;
#endif
        return;
    }

#ifdef DEPLOY_VIA_MOUNT
    Q_ASSERT(!m_currentDeployAction);
    Q_ASSERT(!m_needsInstall);
    Q_ASSERT(m_filesToCopy.isEmpty());
    const MaemoPackageCreationStep * const pStep = packagingStep();
    const QString hostName = m_connection->connectionParameters().host;
    if (pStep->isPackagingEnabled()) {
        const MaemoDeployable d(pStep->packageFilePath(), QString());
        if (currentlyNeedsDeployment(hostName, d))
            m_needsInstall = true;
    } else {
        const int deployableCount = m_deployables->deployableCount();
        for (int i = 0; i < deployableCount; ++i) {
            const MaemoDeployable &d = m_deployables->deployableAt(i);
            if (currentlyNeedsDeployment(hostName, d))
                m_filesToCopy << d;
        }
    }

    if (m_needsInstall || !m_filesToCopy.isEmpty()) {
        m_mounter->setConnection(m_connection);
        m_mounter->unmount(); // Clean up potential remains.
    } else {
        writeOutput(tr("All files up to date, no installation necessary."));
        emit done();
    }
#else
    // TODO: If nothing to deploy, skip this step.
    m_uploader = m_connection->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()), this,
        SLOT(handleSftpChannelInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
        SLOT(handleSftpChannelInitializationFailed(QString)));
    connect(m_uploader.data(), SIGNAL(finished(Core::SftpJobId, QString)),
        this, SLOT(handleSftpJobFinished(Core::SftpJobId, QString)));
    m_uploader->initialize();
#endif
}

void MaemoDeployStep::handleConnectionFailure()
{
    m_connecting = false;
    if (m_stopped) {
#ifdef DEPLOY_VIA_MOUNT
        m_canStart = true;
#endif
        return;
    }

    raiseError(tr("Could not connect to host: %1")
        .arg(m_connection->errorString()));
}

#ifndef DEPLOY_VIA_MOUNT
void MaemoDeployStep::handleSftpChannelInitialized()
{
    if (m_stopped)
        return;

    m_uploadsInProgress.clear();
    m_linksInProgress.clear();
    m_needsInstall = false;
    const MaemoPackageCreationStep * const pStep = packagingStep();
    const QString hostName = m_connection->connectionParameters().host;
    if (pStep->isPackagingEnabled()) {
        const MaemoDeployable d(pStep->packageFilePath(), uploadDir());
        if (currentlyNeedsDeployment(hostName, d)) {
            if (!deploy(MaemoDeployable(d)))
                return;
            m_needsInstall = true;
        } else {
            m_needsInstall = false;
        }
    } else {
        const int deployableCount = m_deployables->deployableCount();
        for (int i = 0; i < deployableCount; ++i) {
            const MaemoDeployable &d = m_deployables->deployableAt(i);
            if (currentlyNeedsDeployment(hostName, d)
                && !deploy(MaemoDeployable(d)))
                return;
        }
        m_needsInstall = false;
    }
    if (m_uploadsInProgress.isEmpty())
        emit done();
}

bool MaemoDeployStep::deploy(const MaemoDeployable &deployable)
{
    const QString fileName = QFileInfo(deployable.localFilePath).fileName();
    const QString remoteFilePath = deployable.remoteDir + '/' + fileName;
    const QString uploadFilePath = uploadDir() + '/' + fileName + '.'
        + QCryptographicHash::hash(remoteFilePath.toUtf8(),
              QCryptographicHash::Md5).toHex();
    const SftpJobId job = m_uploader->uploadFile(deployable.localFilePath,
        uploadFilePath, SftpOverwriteExisting);
    if (job == SftpInvalidJob) {
        raiseError(tr("Upload failed: Could not open file '%1'")
            .arg(deployable.localFilePath));
        return false;
    }
    writeOutput(tr("Started uploading file '%1'.").arg(deployable.localFilePath));
    m_uploadsInProgress.insert(job, DeployInfo(deployable, uploadFilePath));
    return true;
}

void MaemoDeployStep::handleSftpChannelInitializationFailed(const QString &error)
{
    if (m_stopped)
        return;
    raiseError(tr("Could not set up SFTP connection: %1").arg(error));
}

void MaemoDeployStep::handleSftpJobFinished(Core::SftpJobId job,
    const QString &error)
{
    if (m_stopped)
        return;

    QMap<SftpJobId, DeployInfo>::Iterator it = m_uploadsInProgress.find(job);
    if (it == m_uploadsInProgress.end()) {
        qWarning("%s: Job %u not found in map.", Q_FUNC_INFO, job);
        return;
    }

    const DeployInfo &deployInfo = it.value();
    if (!error.isEmpty()) {
        raiseError(tr("Failed to upload file %1: %2")
            .arg(deployInfo.first.localFilePath, error));
        return;
    }

    writeOutput(tr("Successfully uploaded file '%1'.")
        .arg(deployInfo.first.localFilePath));
    const QString remoteFilePath = deployInfo.first.remoteDir + '/'
        + QFileInfo(deployInfo.first.localFilePath).fileName();
    QByteArray linkCommand = MaemoGlobal::remoteSudo().toUtf8() + " ln -sf "
        + deployInfo.second.toUtf8() + ' ' + remoteFilePath.toUtf8();
    SshRemoteProcess::Ptr linkProcess
        = m_connection->createRemoteProcess(linkCommand);
    connect(linkProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleLinkProcessFinished(int)));
    m_linksInProgress.insert(linkProcess, deployInfo.first);
    linkProcess->start();
    m_uploadsInProgress.erase(it);
}

void MaemoDeployStep::handleLinkProcessFinished(int exitStatus)
{
    if (m_stopped)
        return;

    SshRemoteProcess * const proc = static_cast<SshRemoteProcess *>(sender());

    // TODO: List instead of map? We can't use it for lookup anyway.
    QMap<SshRemoteProcess::Ptr, MaemoDeployable>::Iterator it;
    for (it = m_linksInProgress.begin(); it != m_linksInProgress.end(); ++it) {
        if (it.key().data() == proc)
            break;
    }
    if (it == m_linksInProgress.end()) {
        qWarning("%s: Remote process %p not found in process list.",
            Q_FUNC_INFO, proc);
        return;
    }

    const MaemoDeployable &deployable = it.value();
    if (exitStatus != SshRemoteProcess::ExitedNormally
        || proc->exitCode() != 0) {
        raiseError(tr("Deployment failed for file '%1': "
            "Could not create link '%2' on remote system.")
            .arg(deployable.localFilePath, deployable.remoteDir + '/'
                + QFileInfo(deployable.localFilePath).fileName()));
        return;
    }

    setDeployed(m_connection->connectionParameters().host, it.value());
    m_linksInProgress.erase(it);
    if (m_linksInProgress.isEmpty() && m_uploadsInProgress.isEmpty()) {
        if (m_needsInstall) {
            writeOutput(tr("Installing package ..."));
            const QString packageFileName
                = QFileInfo(packagingStep()->packageFilePath()).fileName();
            const QByteArray cmd = MaemoGlobal::remoteSudo().toUtf8()
                + " dpkg -i " + packageFileName.toUtf8();
            m_installer = m_connection->createRemoteProcess(cmd);
            connect(m_installer.data(), SIGNAL(closed(int)), this,
                SLOT(handleInstallationFinished(int)));
            connect(m_installer.data(), SIGNAL(outputAvailable(QByteArray)),
                this, SLOT(handleInstallerOutput(QByteArray)));
            connect(m_installer.data(),
                SIGNAL(errorOutputAvailable(QByteArray)), this,
                SLOT(handleInstallerErrorOutput(QByteArray)));
            m_installer->start();
        } else {
            emit done();
        }
    }
}
#endif

#ifdef DEPLOY_VIA_MOUNT
void MaemoDeployStep::handleMounted()
{
    if (m_stopped) {
        m_mounter->unmount();
        return;
    }

    if (m_needsInstall) {
        writeOutput(tr("Installing package ..."));
        const QString packageFileName
            = QFileInfo(packagingStep()->packageFilePath()).fileName();
        const QByteArray cmd = MaemoGlobal::remoteSudo().toUtf8() + " dpkg -i "
            + deployMountPoint().toUtf8() + '/' + packageFileName.toUtf8();
        m_installer = m_connection->createRemoteProcess(cmd);
        connect(m_installer.data(), SIGNAL(closed(int)), this,
            SLOT(handleInstallationFinished(int)));
        connect(m_installer.data(), SIGNAL(outputAvailable(QByteArray)),
            this, SLOT(handleInstallerOutput(QByteArray)));
        connect(m_installer.data(),
            SIGNAL(errorOutputAvailable(QByteArray)), this,
            SLOT(handleInstallerErrorOutput(QByteArray)));
        m_installer->start();
    } else {
        deployNextFile();
    }
}

void MaemoDeployStep::handleUnmounted()
{
    if (m_stopped) {
        m_canStart = true;
        return;
    }

    if (m_needsInstall || !m_filesToCopy.isEmpty()) {
        m_mounter->setPortList(deviceConfig().freePorts());
        if (m_needsInstall) {
            const QString localDir = QFileInfo(packagingStep()->packageFilePath())
                .absolutePath();
            const MaemoMountSpecification mountSpec(localDir,
                deployMountPoint());
            if (!addMountSpecification(mountSpec))
                return;
        } else {
#ifdef Q_OS_WIN
            bool drivesToMount[26];
            for (int i = 0; i < sizeof drivesToMount / drivesToMount[0]; ++i)
                drivesToMount[i] = false;
            for (int i = 0; i < m_filesToCopy.count(); ++i) {
                const QString localDir = QFileInfo(m_filesToCopy.at(i).localFilePath)
                    .canonicalPath();
                const char driveLetter = localDir.at(0).toLower().toLatin1();
                if (driveLetter < 'a' || driveLetter > 'z') {
                    qWarning("Weird: drive letter is '%c'.", driveLetter);
                    continue;
                }

                const int index = driveLetter - 'a';
                if (drivesToMount[index])
                    continue;

                const QString mountPoint = deployMountPoint()
                    + QLatin1Char('/') + QLatin1Char(driveLetter);
                const MaemoMountSpecification mountSpec(localDir.left(3),
                    mountPoint);
                if (!addMountSpecification(mountSpec))
                    return;
                drivesToMount[index] = true;
            }
#else
            if (!addMountSpecification(MaemoMountSpecification(QLatin1String("/"),
                deployMountPoint())))
                return;
#endif
        }
        m_mounter->mount();
    } else {
        writeOutput(tr("Deployment finished."));
        emit done();
    }
}

void MaemoDeployStep::handleMountError(const QString &errorMsg)
{
    if (m_stopped)
        m_canStart = true;
    else
        raiseError(errorMsg);
}

void MaemoDeployStep::handleProgressReport(const QString &progressMsg)
{
    writeOutput(progressMsg);
}

void MaemoDeployStep::deployNextFile()
{
    Q_ASSERT(!m_filesToCopy.isEmpty());
    Q_ASSERT(!m_currentDeployAction);
    const MaemoDeployable d = m_filesToCopy.takeFirst();
    QString sourceFilePath = deployMountPoint();
#ifdef Q_OS_WIN
    const QString localFilePath = QDir::fromNativeSeparators(d.localFilePath);
    sourceFilePath += QLatin1Char('/') + localFilePath.at(0).toLower()
        + localFilePath.mid(2);
#else
    sourceFilePath += d.localFilePath;
#endif

    QString command = QString::fromLatin1("%1 cp -r %2 %3")
        .arg(MaemoGlobal::remoteSudo(), sourceFilePath,
            d.remoteDir + QLatin1Char('/'));
    SshRemoteProcess::Ptr copyProcess
        = m_connection->createRemoteProcess(command.toUtf8());
    connect(copyProcess.data(), SIGNAL(errorOutputAvailable(QByteArray)),
        this, SLOT(handleInstallerErrorOutput(QByteArray)));
    connect(copyProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleCopyProcessFinished(int)));
    m_currentDeployAction.reset(new DeployAction(d, copyProcess));
    writeOutput(tr("Copying file '%1' to path '%2' on the device...")
        .arg(d.localFilePath, d.remoteDir));
    copyProcess->start();
}

bool MaemoDeployStep::addMountSpecification(const MaemoMountSpecification &mountSpec)
{
    if (!m_mounter->addMountSpecification(mountSpec, true)) {
        raiseError(tr("Device has not enough free ports for deployment."));
        return false;
    }
    return true;
}

void MaemoDeployStep::handleCopyProcessFinished(int exitStatus)
{
    if (m_stopped) {
        m_mounter->unmount();
        return;
    }

    Q_ASSERT(m_currentDeployAction);
    const QString localFilePath = m_currentDeployAction->first.localFilePath;
    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_currentDeployAction->second->exitCode() != 0) {
        raiseError(tr("Copying file '%1' failed.").arg(localFilePath));
        m_mounter->unmount();
        m_currentDeployAction.reset(0);
    } else {
        writeOutput(tr("Successfully copied file '%1'.").arg(localFilePath));
        setDeployed(m_connection->connectionParameters().host,
            m_currentDeployAction->first);
        m_currentDeployAction.reset(0);
        if (m_filesToCopy.isEmpty()) {
            writeOutput(tr("All files copied."));
            m_mounter->unmount();
        } else {
            deployNextFile();
        }
    }
}

void MaemoDeployStep::handleCleanupTimeout()
{
    if (!m_canStart) {
        qWarning("%s: Deployment cleanup failed apparently, "
            "explicitly enabling re-start.", Q_FUNC_INFO);
        m_canStart = true;
        disconnect(m_connection.data(), 0, this, 0);
        if (m_installer)
            disconnect(m_installer.data(), 0, this, 0);
        if (m_currentDeployAction)
            disconnect(m_currentDeployAction->second.data(), 0, this, 0);
    }
}

QString MaemoDeployStep::deployMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(deviceConfig().server.uname)
        + QLatin1String("/deployMountPoint");
}
#endif

void MaemoDeployStep::handleInstallationFinished(int exitStatus)
{
#ifdef DEPLOY_VIA_MOUNT
    if (m_stopped) {
        m_mounter->unmount();
        return;
    }

    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_installer->exitCode() != 0) {
        raiseError(tr("Installing package failed."));
    } else {
        m_needsInstall = false;
        setDeployed(m_connection->connectionParameters().host,
            MaemoDeployable(packagingStep()->packageFilePath(), QString()));
        writeOutput(tr("Package installed."));
    }
    m_mounter->unmount();
#else
    if (m_stopped)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_installer->exitCode() != 0) {
        raiseError(tr("Installing package failed."));
    } else {
        writeOutput(tr("Package installation finished."));
        emit done();
    }
#endif
}

void MaemoDeployStep::handleInstallerOutput(const QByteArray &output)
{
    writeOutput(QString::fromUtf8(output));
}

void MaemoDeployStep::handleInstallerErrorOutput(const QByteArray &output)
{
    writeOutput(output, BuildStep::ErrorOutput);
}


MaemoDeployEventHandler::MaemoDeployEventHandler(MaemoDeployStep *deployStep,
    QFutureInterface<bool> &future)
    : m_deployStep(deployStep), m_future(future), m_eventLoop(new QEventLoop)
{
    connect(m_deployStep, SIGNAL(done()), this, SLOT(handleDeployingDone()));
    connect(m_deployStep, SIGNAL(error()), this, SLOT(handleDeployingFailed()));
    QTimer cancelChecker;
    connect(&cancelChecker, SIGNAL(timeout()), this, SLOT(checkForCanceled()));
    cancelChecker.start(500);
    future.reportResult(m_eventLoop->exec() == 0);
}

void MaemoDeployEventHandler::handleDeployingDone()
{
    m_eventLoop->exit(0);
}

void MaemoDeployEventHandler::handleDeployingFailed()
{
    m_eventLoop->exit(1);
}

void MaemoDeployEventHandler::checkForCanceled()
{
    if (m_future.isCanceled())
        handleDeployingFailed();
}

} // namespace Internal
} // namespace Qt4ProjectManager
