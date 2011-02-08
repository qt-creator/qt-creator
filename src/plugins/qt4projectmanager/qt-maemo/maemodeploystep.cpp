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

#include "maemodeploystep.h"

#include "maemoconstants.h"
#include "maemodeploystepwidget.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemopertargetdeviceconfigurationlistmodel.h"
#include "maemoqemumanager.h"
#include "maemoremotemounter.h"
#include "maemorunconfiguration.h"
#include "maemotoolchain.h"
#include "maemousedportsgatherer.h"
#include "qt4maemotarget.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Core;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {
namespace { const int DefaultMountPort = 1050; }

const QLatin1String MaemoDeployStep::Id("Qt4ProjectManager.MaemoDeployStep");

MaemoDeployStep::MaemoDeployStep(ProjectExplorer::BuildStepList *parent)
    : BuildStep(parent, Id)
{
    ctor();
}

MaemoDeployStep::MaemoDeployStep(ProjectExplorer::BuildStepList *parent,
    MaemoDeployStep *other)
    : BuildStep(parent, other), m_lastDeployed(other->m_lastDeployed)
{
    ctor();
}

MaemoDeployStep::~MaemoDeployStep() { }

void MaemoDeployStep::ctor()
{
    //: MaemoDeployStep default display name
    if (target()->id() == QLatin1String(Constants::MAEMO5_DEVICE_TARGET_ID))
        setDefaultDisplayName(tr("Deploy to Maemo5 device"));
    else if (target()->id() == QLatin1String(Constants::HARMATTAN_DEVICE_TARGET_ID))
        setDefaultDisplayName(tr("Deploy to Harmattan device"));
    else if (target()->id() == QLatin1String(Constants::MEEGO_DEVICE_TARGET_ID))
        setDefaultDisplayName(tr("Deploy to Meego device"));

    // A MaemoDeployables object is only dependent on the active build
    // configuration and therefore can (and should) be shared among all
    // deploy steps.
    const QList<DeployConfiguration *> &deployConfigs
        = target()->deployConfigurations();
    if (deployConfigs.isEmpty()) {
        const AbstractQt4MaemoTarget * const qt4Target = qobject_cast<AbstractQt4MaemoTarget *>(target());
        Q_ASSERT(qt4Target);
        m_deployables = QSharedPointer<MaemoDeployables>(new MaemoDeployables(qt4Target));
    } else {
        const MaemoDeployStep *const other
            = MaemoGlobal::buildStep<MaemoDeployStep>(deployConfigs.first());
        m_deployables = other->deployables();
    }

    m_state = Inactive;
    m_deviceConfig = maemotarget()->deviceConfigurationsModel()->defaultDeviceConfig();
    m_needsInstall = false;
    m_sysrootInstaller = new QProcess(this);
    connect(m_sysrootInstaller, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(handleSysrootInstallerFinished()));
    connect(m_sysrootInstaller, SIGNAL(readyReadStandardOutput()), this,
        SLOT(handleSysrootInstallerOutput()));
    connect(m_sysrootInstaller, SIGNAL(readyReadStandardError()), this,
        SLOT(handleSysrootInstallerErrorOutput()));
    m_mounter = new MaemoRemoteMounter(this);
    connect(m_mounter, SIGNAL(mounted()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SLOT(handleProgressReport(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), this,
        SLOT(handleMountDebugOutput(QString)));
    m_portsGatherer = new MaemoUsedPortsGatherer(this);
    connect(m_portsGatherer, SIGNAL(error(QString)), this,
        SLOT(handlePortsGathererError(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()), this,
        SLOT(handlePortListReady()));
    connect(maemotarget()->deviceConfigurationsModel(), SIGNAL(updated()),
        SLOT(handleDeviceConfigurationsUpdated()));
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
    map.insert(DeviceIdKey,
        MaemoDeviceConfigurations::instance()->internalId(m_deviceConfig));
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
    setDeviceConfig(map.value(DeviceIdKey, MaemoDeviceConfig::InvalidId).toULongLong());
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
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    emit error();
}

void MaemoDeployStep::writeOutput(const QString &text, OutputFormat format)
{
    emit addOutput(text, format);
}

void MaemoDeployStep::stop()
{
    if (m_state == StopRequested || m_state == Inactive)
        return;

    const State oldState = m_state;
    setState(StopRequested);
    switch (oldState) {
    case InstallingToSysroot:
        if (m_needsInstall)
            m_sysrootInstaller->terminate();
        break;
    case Connecting:
        m_connection->disconnectFromHost();
        setState(Inactive);
        break;
    case InstallingToDevice:
    case CopyingFile: {
        const QByteArray programToKill = oldState == CopyingFile
            ? " cp " : "dpkg";
        const QByteArray killCommand
            = MaemoGlobal::remoteSudo().toUtf8() + " pkill -f ";
        const QByteArray cmdLine = killCommand + programToKill + "; sleep 1; "
            + killCommand + "-9 " + programToKill;
        SshRemoteProcess::Ptr killProc
            = m_connection->createRemoteProcess(cmdLine);
        killProc->start();
        break;
    }
    case Uploading:
        m_uploader->closeChannel();
        break;
    case UnmountingOldDirs:
    case UnmountingCurrentDirs:
    case UnmountingCurrentMounts:
    case GatheringPorts:
    case Mounting:
    case InitializingSftp:
        break; // Nothing to do here.
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Missing switch case.");
    }
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

void MaemoDeployStep::handleDeviceConfigurationsUpdated()
{
    setDeviceConfig(MaemoDeviceConfigurations::instance()->internalId(m_deviceConfig));
}

void MaemoDeployStep::setDeviceConfig(MaemoDeviceConfig::Id internalId)
{
    m_deviceConfig = maemotarget()->deviceConfigurationsModel()->find(internalId);
    emit deviceConfigChanged();
}

void MaemoDeployStep::setDeviceConfig(int i)
{
    m_deviceConfig = maemotarget()->deviceConfigurationsModel()->deviceAt(i);
    emit deviceConfigChanged();
}

void MaemoDeployStep::start()
{
    if (m_state != Inactive) {
        raiseError(tr("Cannot deploy: Still cleaning up from last time."));
        emit done();
        return;
    }

    m_cachedDeviceConfig = m_deviceConfig;
    if (!m_cachedDeviceConfig) {
        raiseError(tr("Deployment failed: No valid device set."));
        emit done();
        return;
    }

    Q_ASSERT(!m_currentDeviceDeployAction);
    Q_ASSERT(!m_needsInstall);
    Q_ASSERT(m_filesToCopy.isEmpty());
    m_installerStderr.clear();
    const MaemoPackageCreationStep * const pStep = packagingStep();
    const QString hostName = m_cachedDeviceConfig->sshParameters().host;
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
        if (m_cachedDeviceConfig->type() == MaemoDeviceConfig::Simulator
                && !MaemoQemuManager::instance().qemuIsRunning()) {
            MaemoQemuManager::instance().startRuntime();
            raiseError(tr("Deployment failed: Qemu was not running. "
                "It has now been started up for you, but it will take "
                "a bit of time until it is ready."));
            m_needsInstall = false;
            m_filesToCopy.clear();
            emit done();
            return;
        }

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
    if (m_state == Inactive)
        return;

    const QString errorMsg = m_state == Connecting
        ? MaemoGlobal::failedToConnectToServerMessage(m_connection, m_cachedDeviceConfig)
        : tr("Connection error: %1").arg(m_connection->errorString());
    raiseError(errorMsg);
    setState(Inactive);
}

void MaemoDeployStep::handleSftpChannelInitialized()
{
    ASSERT_STATE(QList<State>() << InitializingSftp << StopRequested);

    switch (m_state) {
    case InitializingSftp: {
        const QString filePath = packagingStep()->packageFilePath();
        const QString filePathNative = QDir::toNativeSeparators(filePath);
        const QString fileName = QFileInfo(filePath).fileName();
        const QString remoteFilePath = uploadDir() + QLatin1Char('/') + fileName;
        const SftpJobId job = m_uploader->uploadFile(filePath,
            remoteFilePath, SftpOverwriteExisting);
        if (job == SftpInvalidJob) {
            raiseError(tr("Upload failed: Could not open file '%1'")
                .arg(filePathNative));
            setState(Inactive);
        } else {
            setState(Uploading);
            writeOutput(tr("Started uploading file '%1'.").arg(filePathNative));
        }
        break;
    }
    case StopRequested:
        setState(Inactive);
        break;
    default:
        break;
    }
}

void MaemoDeployStep::handleSftpChannelInitializationFailed(const QString &error)
{
    ASSERT_STATE(QList<State>() << InitializingSftp << StopRequested);

    switch (m_state) {
    case InitializingSftp:
    case StopRequested:
        raiseError(tr("Could not set up SFTP connection: %1").arg(error));
        setState(Inactive);
        break;
    default:
        break;
    }
}

void MaemoDeployStep::handleSftpJobFinished(Core::SftpJobId,
    const QString &error)
{
    ASSERT_STATE(QList<State>() << Uploading << StopRequested);

    const QString filePathNative
        = QDir::toNativeSeparators(packagingStep()->packageFilePath());
    if (!error.isEmpty()) {
        raiseError(tr("Failed to upload file %1: %2")
            .arg(filePathNative, error));
        if (m_state == Uploading)
            setState(Inactive);
    } else if (m_state == Uploading) {
        writeOutput(tr("Successfully uploaded file '%1'.")
            .arg(filePathNative));
        const QString remoteFilePath
            = uploadDir() + QLatin1Char('/') + QFileInfo(filePathNative).fileName();
        runPackageInstaller(remoteFilePath);
    }
}

void MaemoDeployStep::handleSftpChannelClosed()
{
    ASSERT_STATE(StopRequested);
    setState(Inactive);
}

void MaemoDeployStep::handleMounted()
{
    ASSERT_STATE(QList<State>() << Mounting << StopRequested << Inactive);

    switch (m_state) {
    case Mounting:
        if (m_needsInstall) {
            const QString remoteFilePath = deployMountPoint() + QLatin1Char('/')
                + QFileInfo(packagingStep()->packageFilePath()).fileName();
            runPackageInstaller(remoteFilePath);
        } else {
            setState(CopyingFile);
            copyNextFileToDevice();
        }
        break;
    case StopRequested:
        unmount();
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::handleUnmounted()
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << StopRequested << Inactive);

    switch (m_state) {
    case StopRequested:
        m_mounter->resetMountSpecifications();
        setState(Inactive);
        break;
    case UnmountingOldDirs:
        if (maemotarget()->allowsRemoteMounts())
            setupMount();
        else
            prepareSftpConnection();
        break;
    case UnmountingCurrentDirs:
        setState(GatheringPorts);
        m_portsGatherer->start(m_connection, m_cachedDeviceConfig->freePorts());
        break;
    case UnmountingCurrentMounts:
        writeOutput(tr("Deployment finished."));
        setState(Inactive);
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::handleMountError(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << Mounting << StopRequested << Inactive);

    switch (m_state) {
    case UnmountingOldDirs:
    case UnmountingCurrentDirs:
    case UnmountingCurrentMounts:
    case Mounting:
    case StopRequested:
        raiseError(errorMsg);
        setState(Inactive);
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::handleMountDebugOutput(const QString &output)
{
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << Mounting << StopRequested << Inactive);

    switch (m_state) {
    case UnmountingOldDirs:
    case UnmountingCurrentDirs:
    case UnmountingCurrentMounts:
    case Mounting:
    case StopRequested:
        writeOutput(output, ErrorOutput);
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::setupMount()
{
    ASSERT_STATE(UnmountingOldDirs);
    setState(UnmountingCurrentDirs);

    Q_ASSERT(m_needsInstall || !m_filesToCopy.isEmpty());
    m_mounter->resetMountSpecifications();
    m_mounter->setBuildConfiguration(static_cast<Qt4BuildConfiguration *>(buildConfiguration()));
    if (m_needsInstall) {
        const QString localDir
            = QFileInfo(packagingStep()->packageFilePath()).absolutePath();
        const MaemoMountSpecification mountSpec(localDir, deployMountPoint());
        m_mounter->addMountSpecification(mountSpec, true);
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
            m_mounter->addMountSpecification(mountSpec, true);
            drivesToMount[index] = true;
        }
#else
        m_mounter->addMountSpecification(MaemoMountSpecification(QLatin1String("/"),
            deployMountPoint()), true);
#endif
    }
    unmount();
}

void MaemoDeployStep::prepareSftpConnection()
{
    setState(InitializingSftp);
    m_uploader = m_connection->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()), this,
        SLOT(handleSftpChannelInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
        SLOT(handleSftpChannelInitializationFailed(QString)));
    connect(m_uploader.data(), SIGNAL(finished(Core::SftpJobId, QString)),
        this, SLOT(handleSftpJobFinished(Core::SftpJobId, QString)));
    connect(m_uploader.data(), SIGNAL(closed()), this,
        SLOT(handleSftpChannelClosed()));
    m_uploader->initialize();
}

void MaemoDeployStep::installToSysroot()
{
    ASSERT_STATE(Inactive);
    setState(InstallingToSysroot);

    if (m_needsInstall) {
        writeOutput(tr("Installing package to sysroot ..."));
        const Qt4BuildConfiguration * const bc
            = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
        const QtVersion * const qtVersion = bc->qtVersion();
        const QString command = QLatin1String(
            packagingStep()->debBasedMaemoTarget() ? "xdpkg" : "xrpm");
        QStringList args = QStringList() << command << QLatin1String("-i");
        if (packagingStep()->debBasedMaemoTarget())
            args << QLatin1String("--no-force-downgrade");
        args << packagingStep()->packageFilePath();
        MaemoGlobal::callMadAdmin(*m_sysrootInstaller, args, qtVersion, true);
        if (!m_sysrootInstaller->waitForStarted()) {
            writeOutput(tr("Installation to sysroot failed, continuing anyway."),
                ErrorMessageOutput);
            connectToDevice();
        }
    } else {
        writeOutput(tr("Copying files to sysroot ..."));
        Q_ASSERT(!m_filesToCopy.isEmpty());
        QDir sysRootDir(toolChain()->sysroot());
        foreach (const MaemoDeployable &d, m_filesToCopy) {
            const QLatin1Char sep('/');
            const QString targetFilePath = toolChain()->sysroot() + sep
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
            if (m_state == StopRequested) {
                setState(Inactive);
                return;
            }
        }
        connectToDevice();
    }
}

void MaemoDeployStep::handleSysrootInstallerFinished()
{
    ASSERT_STATE(QList<State>() << InstallingToSysroot << StopRequested);

    if (m_state == StopRequested) {
        setState(Inactive);
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
    ASSERT_STATE(QList<State>() << Inactive << InstallingToSysroot);
    setState(Connecting);

    const bool canReUse = m_connection
        && m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == m_cachedDeviceConfig->sshParameters();
    if (!canReUse)
        m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(Core::SshError)), this,
        SLOT(handleConnectionFailure()));
    if (canReUse) {
        handleConnected();
    } else {
        writeOutput(tr("Connecting to device..."));
        m_connection->connectToHost(m_cachedDeviceConfig->sshParameters());
    }
}

void MaemoDeployStep::handleConnected()
{
    ASSERT_STATE(QList<State>() << Connecting << StopRequested);

    if (m_state == Connecting)
        unmountOldDirs();
}

void MaemoDeployStep::unmountOldDirs()
{
    setState(UnmountingOldDirs);
    m_mounter->setConnection(m_connection);
    unmount();
}

void MaemoDeployStep::runPackageInstaller(const QString &packageFilePath)
{
    ASSERT_STATE(QList<State>() << Mounting << Uploading);
    const bool removeAfterInstall = m_state == Uploading;
    setState(InstallingToDevice);

    writeOutput(tr("Installing package to device..."));
    const QByteArray installCommand = packagingStep()->debBasedMaemoTarget()
        ? "dpkg -i --no-force-downgrade" : "rpm -Uhv";
    QByteArray cmd = MaemoGlobal::remoteSudo().toUtf8() + ' '
        + installCommand + ' ' + packageFilePath.toUtf8();
    if (removeAfterInstall)
        cmd += " && (rm " + packageFilePath.toUtf8() + " || :)";
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
    ASSERT_STATE(QList<State>() << UnmountingOldDirs << UnmountingCurrentDirs
        << UnmountingCurrentMounts << Mounting << StopRequested << Inactive);

    switch (m_state) {
    case UnmountingOldDirs:
    case UnmountingCurrentDirs:
    case UnmountingCurrentMounts:
    case Mounting:
    case StopRequested:
        writeOutput(progressMsg);
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::copyNextFileToDevice()
{
    ASSERT_STATE(CopyingFile);
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

void MaemoDeployStep::handleCopyProcessFinished(int exitStatus)
{
    ASSERT_STATE(QList<State>() << CopyingFile << StopRequested << Inactive);

    switch (m_state) {
    case CopyingFile: {
        Q_ASSERT(m_currentDeviceDeployAction);
        const QString localFilePath
            = m_currentDeviceDeployAction->first.localFilePath;
        if (exitStatus != SshRemoteProcess::ExitedNormally
                || m_currentDeviceDeployAction->second->exitCode() != 0) {
            raiseError(tr("Copying file '%1' failed.").arg(localFilePath));
            m_currentDeviceDeployAction.reset(0);
            setState(UnmountingCurrentMounts);
            unmount();
        } else {
            writeOutput(tr("Successfully copied file '%1'.").arg(localFilePath));
            setDeployed(m_connection->connectionParameters().host,
                m_currentDeviceDeployAction->first);
            m_currentDeviceDeployAction.reset(0);
            if (m_filesToCopy.isEmpty()) {
                writeOutput(tr("All files copied."));
                setState(UnmountingCurrentMounts);
                unmount();
            } else {
                copyNextFileToDevice();
            }
        }
        break;
    }
    case StopRequested:
        unmount();
        break;
    case Inactive:
    default:
        break;
    }
}

QString MaemoDeployStep::deployMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(m_cachedDeviceConfig->sshParameters().uname)
        + QLatin1String("/deployMountPoint_") + packagingStep()->projectName();
}

const AbstractMaemoToolChain *MaemoDeployStep::toolChain() const
{
    const Qt4BuildConfiguration * const bc
        = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    return static_cast<AbstractMaemoToolChain *>(bc->toolChain());
}

const AbstractQt4MaemoTarget *MaemoDeployStep::maemotarget() const
{
    const Qt4BuildConfiguration * const bc
        = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    return static_cast<AbstractQt4MaemoTarget *>(bc->target());
}

void MaemoDeployStep::handleSysrootInstallerOutput()
{
    ASSERT_STATE(QList<State>() << InstallingToSysroot << StopRequested);

    switch (m_state) {
    case InstallingToSysroot:
    case StopRequested:
        writeOutput(QString::fromLocal8Bit(m_sysrootInstaller->readAllStandardOutput()),
            NormalOutput);
        break;
    default:
        break;
    }
}

void MaemoDeployStep::handleSysrootInstallerErrorOutput()
{
    ASSERT_STATE(QList<State>() << InstallingToSysroot << StopRequested);

    switch (m_state) {
    case InstallingToSysroot:
    case StopRequested:
        writeOutput(QString::fromLocal8Bit(m_sysrootInstaller->readAllStandardError()),
            BuildStep::ErrorOutput);
        break;
    default:
        break;
    }
}

void MaemoDeployStep::handleInstallationFinished(int exitStatus)
{
    ASSERT_STATE(QList<State>() << InstallingToDevice << StopRequested
        << Inactive);

    switch (m_state) {
    case InstallingToDevice:
        if (exitStatus != SshRemoteProcess::ExitedNormally
            || m_deviceInstaller->exitCode() != 0) {
            raiseError(tr("Installing package failed."));
        } else if (m_installerStderr.contains("Will not downgrade")) {
            raiseError(tr("Installation failed: "
                "You tried to downgrade a package, which is not allowed."));
        } else {
            m_needsInstall = false;
            setDeployed(m_connection->connectionParameters().host,
                MaemoDeployable(packagingStep()->packageFilePath(), QString()));
            writeOutput(tr("Package installed."));
        }
        setState(UnmountingCurrentMounts);
        unmount();
        break;
    case StopRequested:
        unmount();
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::handlePortsGathererError(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << GatheringPorts << StopRequested << Inactive);

    if (m_state != Inactive) {
        raiseError(errorMsg);
        setState(Inactive);
    }
}

void MaemoDeployStep::handlePortListReady()
{
    ASSERT_STATE(QList<State>() << GatheringPorts << StopRequested);

    if (m_state == GatheringPorts) {
        setState(Mounting);
        m_freePorts = m_cachedDeviceConfig->freePorts();
        m_mounter->mount(&m_freePorts, m_portsGatherer);
    } else {
        setState(Inactive);
    }
}

void MaemoDeployStep::setState(State newState)
{
    if (newState == m_state)
        return;
    m_state = newState;
    if (m_state == Inactive) {
        m_needsInstall = false;
        m_filesToCopy.clear();
        m_currentDeviceDeployAction.reset(0);
        if (m_connection)
            disconnect(m_connection.data(), 0, this, 0);
        if (m_uploader) {
            disconnect(m_uploader.data(), 0, this, 0);
            m_uploader->closeChannel();
        }
        if (m_deviceInstaller)
            disconnect(m_deviceInstaller.data(), 0, this, 0);
        emit done();
    }
}

void MaemoDeployStep::unmount()
{
    if (m_mounter->hasValidMountSpecifications())
        m_mounter->unmount();
    else
        handleUnmounted();
}

void MaemoDeployStep::handleDeviceInstallerOutput(const QByteArray &output)
{
    ASSERT_STATE(QList<State>() << InstallingToDevice << StopRequested);

    switch (m_state) {
    case InstallingToDevice:
    case StopRequested:
        writeOutput(QString::fromUtf8(output), NormalOutput);
        break;
    default:
        break;
    }
}

void MaemoDeployStep::handleDeviceInstallerErrorOutput(const QByteArray &output)
{
    ASSERT_STATE(QList<State>() << InstallingToDevice << StopRequested);

    switch (m_state) {
    case InstallingToDevice:
    case StopRequested:
        m_installerStderr += output;
        writeOutput(QString::fromUtf8(output), ErrorOutput);
        break;
    default:
        break;
    }
}

MaemoDeployEventHandler::MaemoDeployEventHandler(MaemoDeployStep *deployStep,
    QFutureInterface<bool> &future)
    : m_deployStep(deployStep), m_future(future), m_eventLoop(new QEventLoop),
      m_error(false)
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
    m_eventLoop->exit(m_error ? 1 : 0);
}

void MaemoDeployEventHandler::handleDeployingFailed()
{
    m_error = true;
}

void MaemoDeployEventHandler::checkForCanceled()
{
    if (!m_error && m_future.isCanceled()) {
        QMetaObject::invokeMethod(m_deployStep, "stop");
        m_error = true;
        handleDeployingDone();
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
