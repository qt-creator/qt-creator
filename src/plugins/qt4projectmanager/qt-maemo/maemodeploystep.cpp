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

#include <QtCore/QCoreApplication>
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
    //: MaemoDeployStep default display name
    setDefaultDisplayName(tr("Deploy to Maemo device"));

    m_connecting = false;
    m_needsInstall = false;
    m_stopped = false;
    m_deviceConfigModel = new MaemoDeviceConfigListModel(this);
    m_canStart = true;
    m_sysrootInstaller = new QProcess(this);
    connect(m_sysrootInstaller, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(handleSysrootInstallerFinished()));
    connect(m_sysrootInstaller, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleSysrootInstallerOutput()));
    connect(m_sysrootInstaller, SIGNAL(readyReadStandardError()), this,
        SLOT(handleSysrootInstallerErrorOutput()));
    m_cleanupTimer = new QTimer(this);
    connect(m_cleanupTimer, SIGNAL(timeout()), this,
        SLOT(handleCleanupTimeout()));
    m_mounter = new MaemoRemoteMounter(this);
    connect(m_mounter, SIGNAL(mounted()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SLOT(handleProgressReport(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), this,
        SLOT(handleMountDebugOutput(QString)));
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
    map.insert(DeployToSysrootKey, m_deployToSysroot);
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
    m_deployToSysroot = map.value(DeployToSysrootKey, true).toBool();
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
    disconnect(m_connection.data(), 0, this, 0);
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        Constants::TASK_CATEGORY_BUILDSYSTEM));
    emit error();
}

void MaemoDeployStep::writeOutput(const QString &text, OutputFormat format)
{
    emit addOutput(text, format);
}

void MaemoDeployStep::stop()
{
    if (m_stopped)
        return;

    const bool remoteProcessRunning
        = (m_deviceInstaller && m_deviceInstaller->isRunning())
              || m_currentDeviceDeployAction;
    const bool isActive = remoteProcessRunning || m_connecting
        || m_needsInstall || !m_filesToCopy.isEmpty();
    if (!isActive) {
        if (m_connection)
            disconnect(m_connection.data(), 0, this, 0);
        return;
    }

    if (remoteProcessRunning) {
        const QByteArray programToKill
            = m_currentDeviceDeployAction ? "/bin/cp" : "/usr/bin/dpkg";
        const QByteArray cmdLine = "pkill " + programToKill
            + "; sleep 1; pkill -9 " + programToKill;
        SshRemoteProcess::Ptr killProc
            = m_connection->createRemoteProcess(cmdLine);
        killProc->start();
    }
    m_stopped = true;
    m_unmountState = CurrentMountsUnmount;
    m_canStart = false;
    m_needsInstall = false;
    m_filesToCopy.clear();
    m_connecting = false;
    m_sysrootInstaller->terminate();
    m_sysrootInstaller->waitForFinished(500);
    m_sysrootInstaller->kill();
    m_cleanupTimer->start(5000);
    m_mounter->stop();
}

QString MaemoDeployStep::uploadDir() const
{
    return MaemoGlobal::homeDirOnDevice(m_connection->connectionParameters().uname);
}

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

void MaemoDeployStep::start()
{
    if (!m_canStart) {
        raiseError(tr("Cannot start deployment, as the clean-up from the last time has not finished yet."));
        return;
    }
    m_cleanupTimer->stop();
    m_stopped = false;

    if (!deviceConfig().isValid()) {
        raiseError(tr("Deployment failed: No valid device set."));
        return;
    }

    Q_ASSERT(!m_currentDeviceDeployAction);
    Q_ASSERT(!m_needsInstall);
    Q_ASSERT(m_filesToCopy.isEmpty());
    const MaemoPackageCreationStep * const pStep = packagingStep();
    const QString hostName = deviceConfig().server.host;
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
        if (m_deployToSysroot)
            installToSysroot();
        else
            connectToDevice();
    } else {
        writeOutput(tr("All files up to date, no installation necessary."));
        emit done();
    }
}

void MaemoDeployStep::handleConnectionFailure()
{
    m_connecting = false;
    if (m_stopped) {
        m_canStart = true;
        return;
    }
    raiseError(tr("Could not connect to host: %1")
        .arg(m_connection->errorString()));
}

void MaemoDeployStep::handleSftpChannelInitialized()
{
    if (m_stopped) {
        m_canStart = true;
        return;
    }

    const QString filePath = packagingStep()->packageFilePath();
    const QString filePathNative = QDir::toNativeSeparators(filePath);
    const QString fileName = QFileInfo(filePath).fileName();
    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + fileName;
    const SftpJobId job = m_uploader->uploadFile(filePath,
        remoteFilePath, SftpOverwriteExisting);
    if (job == SftpInvalidJob) {
        raiseError(tr("Upload failed: Could not open file '%1'")
            .arg(filePathNative));
    } else {
        writeOutput(tr("Started uploading file '%1'.").arg(filePathNative));
    }
}

void MaemoDeployStep::handleSftpChannelInitializationFailed(const QString &error)
{
    if (m_stopped) {
        m_canStart = true;
        return;
    }
    raiseError(tr("Could not set up SFTP connection: %1").arg(error));
}

void MaemoDeployStep::handleSftpJobFinished(Core::SftpJobId,
    const QString &error)
{
    if (m_stopped) {
        m_canStart = true;
        return;
    }

    const QString filePathNative
        = QDir::toNativeSeparators(packagingStep()->packageFilePath());
    if (!error.isEmpty()) {
        raiseError(tr("Failed to upload file %1: %2")
            .arg(filePathNative, error));
        return;
    }

    writeOutput(tr("Successfully uploaded file '%1'.")
        .arg(filePathNative));
    const QString remoteFilePath
        = uploadDir() + QLatin1Char('/') + QFileInfo(filePathNative).fileName();
    runDpkg(remoteFilePath);
}

void MaemoDeployStep::handleMounted()
{
    if (m_stopped) {
        m_mounter->unmount();
        return;
    }

    if (m_needsInstall) {
        const QString remoteFilePath = deployMountPoint() + QLatin1Char('/')
            + QFileInfo(packagingStep()->packageFilePath()).fileName();
        runDpkg(remoteFilePath);
    } else {
        copyNextFileToDevice();
    }
}

void MaemoDeployStep::handleUnmounted()
{
    if (m_stopped) {
        m_mounter->resetMountSpecifications();
        m_canStart = true;
        return;
    }

    switch (m_unmountState) {
    case OldDirsUnmount:
        if (toolChain()->allowsRemoteMounts())
            setupMount();
        else
            prepareSftpConnection();
        break;
    case CurrentDirsUnmount:
        m_mounter->mount();
        break;
    case CurrentMountsUnmount:
        writeOutput(tr("Deployment finished."));
        emit done();
        break;
    }
}

void MaemoDeployStep::handleMountError(const QString &errorMsg)
{
    if (m_stopped)
        m_canStart = true;
    else
        raiseError(errorMsg);
}

void MaemoDeployStep::handleMountDebugOutput(const QString &output)
{
    if (!m_stopped)
        writeOutput(output, ErrorOutput);
}

void MaemoDeployStep::setupMount()
{
    Q_ASSERT(m_needsInstall || !m_filesToCopy.isEmpty());
    m_mounter->resetMountSpecifications();
    m_mounter->setToolchain(toolChain());
    m_mounter->setPortList(deviceConfig().freePorts());
    if (m_needsInstall) {
        const QString localDir
            = QFileInfo(packagingStep()->packageFilePath()).absolutePath();
        const MaemoMountSpecification mountSpec(localDir, deployMountPoint());
        if (!addMountSpecification(mountSpec))
            return;
    } else {
#ifdef Q_OS_WIN
        bool drivesToMount[26];
        qFill(drivesToMount, drivesToMount + sizeof drivesToMount / sizeof drivesToMount[0], false);
        for (int i = 0; i < m_filesToCopy.count(); ++i) {
            const QString localDir
                = QFileInfo(m_filesToCopy.at(i).localFilePath).canonicalPath();
            const char driveLetter = localDir.at(0).toLower().toLatin1();
            if (driveLetter < 'a' || driveLetter > 'z') {
                qWarning("Weird: drive letter is '%c'.", driveLetter);
                continue;
            }

            const int index = driveLetter - 'a';
            if (drivesToMount[index])
                continue;

            const QString mountPoint = deployMountPoint() + QLatin1Char('/')
                + QLatin1Char(driveLetter);
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
    m_unmountState = CurrentDirsUnmount;
    m_mounter->unmount();
}

void MaemoDeployStep::prepareSftpConnection()
{
    m_uploader = m_connection->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()), this,
        SLOT(handleSftpChannelInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
        SLOT(handleSftpChannelInitializationFailed(QString)));
    connect(m_uploader.data(), SIGNAL(finished(Core::SftpJobId, QString)),
        this, SLOT(handleSftpJobFinished(Core::SftpJobId, QString)));
    m_uploader->initialize();
}

void MaemoDeployStep::installToSysroot()
{
    if (m_needsInstall) {
        writeOutput(tr("Installing package to sysroot ..."));
        const MaemoToolChain * const tc = toolChain();
        const QStringList args = QStringList() << QLatin1String("-t")
            << tc->targetName() << QLatin1String("xdpkg") << QLatin1String("-i")
            << packagingStep()->packageFilePath();
        m_sysrootInstaller->start(tc->madAdminCommand(), args);
        if (!m_sysrootInstaller->waitForStarted()) {
            writeOutput(tr("Installation to sysroot failed, continuing anyway."),
                ErrorMessageOutput);
            connectToDevice();
        }
    } else {
        writeOutput(tr("Copying files to sysroot ..."));
        Q_ASSERT(!m_filesToCopy.isEmpty());
        QDir sysRootDir(toolChain()->sysrootRoot());
        foreach (const MaemoDeployable &d, m_filesToCopy) {
            const QLatin1Char sep('/');
            const QString targetFilePath = toolChain()->sysrootRoot() + sep
                + d.remoteDir + sep + QFileInfo(d.localFilePath).fileName();
            sysRootDir.mkpath(d.remoteDir.mid(1));
            QFile::remove(targetFilePath);
            if (!QFile::copy(d.localFilePath, targetFilePath)) {
                writeOutput(tr("Sysroot installation failed: "
                    "Could not copy '%1' to '%2'. Continuing anyway.")
                    .arg(QDir::toNativeSeparators(d.localFilePath),
                         QDir::toNativeSeparators(targetFilePath)),
                    ErrorMessageOutput);
            }
            QCoreApplication::processEvents();
            if (m_stopped) {
                m_canStart = true;
                return;
            }
        }
        connectToDevice();
    }
}

void MaemoDeployStep::handleSysrootInstallerFinished()
{
    if (m_stopped) {
        m_canStart = true;
        return;
    }

    if (m_sysrootInstaller->error() != QProcess::UnknownError
        || m_sysrootInstaller->exitCode() != 0) {
        writeOutput(tr("Installation to sysroot failed, continuing anyway."),
            ErrorMessageOutput);
    }
    connectToDevice();
}

void MaemoDeployStep::connectToDevice()
{
    m_connecting = false;
    const bool canReUse = m_connection
        && m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == deviceConfig().server;
    if (!canReUse)
        m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(SshError)), this,
        SLOT(handleConnectionFailure()));
    if (canReUse) {
        unmountOldDirs();
    } else {
        writeOutput(tr("Connecting to device..."));
        m_connecting = true;
        m_connection->connectToHost(deviceConfig().server);
    }
}

void MaemoDeployStep::handleConnected()
{
    if (m_stopped) {
        m_canStart = true;
        return;
    }

    unmountOldDirs();
}

void MaemoDeployStep::unmountOldDirs()
{
    m_unmountState = OldDirsUnmount;
    m_mounter->setConnection(m_connection);
    m_mounter->unmount();
}

void MaemoDeployStep::runDpkg(const QString &packageFilePath)
{
    writeOutput(tr("Installing package to device..."));
    const QByteArray cmd = MaemoGlobal::remoteSudo().toUtf8() + " dpkg -i "
        + packageFilePath.toUtf8();
    m_deviceInstaller = m_connection->createRemoteProcess(cmd);
    connect(m_deviceInstaller.data(), SIGNAL(closed(int)), this,
        SLOT(handleInstallationFinished(int)));
    connect(m_deviceInstaller.data(), SIGNAL(outputAvailable(QByteArray)),
        this, SLOT(handleDeviceInstallerOutput(QByteArray)));
    connect(m_deviceInstaller.data(),
        SIGNAL(errorOutputAvailable(QByteArray)), this,
        SLOT(handleDeviceInstallerErrorOutput(QByteArray)));
    m_deviceInstaller->start();
}

void MaemoDeployStep::handleProgressReport(const QString &progressMsg)
{
    writeOutput(progressMsg);
}

void MaemoDeployStep::copyNextFileToDevice()
{
    Q_ASSERT(!m_filesToCopy.isEmpty());
    Q_ASSERT(!m_currentDeviceDeployAction);
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
        this, SLOT(handleDeviceInstallerErrorOutput(QByteArray)));
    connect(copyProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleCopyProcessFinished(int)));
    m_currentDeviceDeployAction.reset(new DeviceDeployAction(d, copyProcess));
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

    Q_ASSERT(m_currentDeviceDeployAction);
    const QString localFilePath = m_currentDeviceDeployAction->first.localFilePath;
    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_currentDeviceDeployAction->second->exitCode() != 0) {
        raiseError(tr("Copying file '%1' failed.").arg(localFilePath));
        m_mounter->unmount();
        m_currentDeviceDeployAction.reset(0);
    } else {
        writeOutput(tr("Successfully copied file '%1'.").arg(localFilePath));
        setDeployed(m_connection->connectionParameters().host,
            m_currentDeviceDeployAction->first);
        m_currentDeviceDeployAction.reset(0);
        if (m_filesToCopy.isEmpty()) {
            writeOutput(tr("All files copied."));
            m_unmountState = CurrentMountsUnmount;
            m_mounter->unmount();
        } else {
            copyNextFileToDevice();
        }
    }
}

void MaemoDeployStep::handleCleanupTimeout()
{
    m_cleanupTimer->stop();
    if (!m_canStart) {
        m_canStart = true;
        disconnect(m_connection.data(), 0, this, 0);
        if (m_deviceInstaller)
            disconnect(m_deviceInstaller.data(), 0, this, 0);
        if (m_currentDeviceDeployAction) {
            disconnect(m_currentDeviceDeployAction->second.data(), 0, this, 0);
            m_currentDeviceDeployAction.reset(0);
        }
    }
}

QString MaemoDeployStep::deployMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(deviceConfig().server.uname)
        + QLatin1String("/deployMountPoint");
}

const MaemoToolChain *MaemoDeployStep::toolChain() const
{
    const Qt4BuildConfiguration * const bc
        = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    return static_cast<MaemoToolChain *>(bc->toolChain());
}

void MaemoDeployStep::handleSysrootInstallerOutput()
{
    if (!m_stopped) {
        writeOutput(QString::fromLocal8Bit(m_sysrootInstaller->readAllStandardOutput()),
            NormalOutput);
    }
}

void MaemoDeployStep::handleSysrootInstallerErrorOutput()
{
    if (!m_stopped) {
        writeOutput(QString::fromLocal8Bit(m_sysrootInstaller->readAllStandardError()),
            BuildStep::ErrorOutput);
    }
}

void MaemoDeployStep::handleInstallationFinished(int exitStatus)
{
    if (m_stopped) {
        m_mounter->unmount();
        return;
    }

    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_deviceInstaller->exitCode() != 0) {
        raiseError(tr("Installing package failed."));
    } else {
        m_needsInstall = false;
        setDeployed(m_connection->connectionParameters().host,
            MaemoDeployable(packagingStep()->packageFilePath(), QString()));
        writeOutput(tr("Package installed."));
    }
    m_unmountState = CurrentMountsUnmount;
    m_mounter->unmount();
}

void MaemoDeployStep::handleDeviceInstallerOutput(const QByteArray &output)
{
    writeOutput(QString::fromUtf8(output), NormalOutput);
}

void MaemoDeployStep::handleDeviceInstallerErrorOutput(const QByteArray &output)
{
    writeOutput(QString::fromUtf8(output), ErrorOutput);
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
