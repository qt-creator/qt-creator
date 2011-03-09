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
#include "maemodeploymentmounter.h"
#include "maemodeploystepwidget.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemopackageinstaller.h"
#include "maemopackageuploader.h"
#include "maemopertargetdeviceconfigurationlistmodel.h"
#include "maemoqemumanager.h"
#include "maemoremotecopyfacility.h"
#include "maemorunconfiguration.h"
#include "maemotoolchain.h"
#include "qt4maemodeployconfiguration.h"
#include "qt4maemotarget.h"

#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshremoteprocess.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4target.h>

#include <utils/ssh/sshconnectionmanager.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

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

    m_state = Inactive;
    m_deviceConfig = maemotarget()->deviceConfigurationsModel()->defaultDeviceConfig();
    m_needsInstall = false;

    m_mounter = new MaemoDeploymentMounter(this);
    connect(m_mounter, SIGNAL(setupDone()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(tearDownDone()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMountError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SLOT(handleProgressReport(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), this,
        SLOT(handleMountDebugOutput(QString)));

    m_uploader = new MaemoPackageUploader(this);
    connect(m_uploader, SIGNAL(progress(QString)),
        SLOT(handleProgressReport(QString)));
    connect(m_uploader, SIGNAL(uploadFinished(QString)),
        SLOT(handleUploadFinished(QString)));

    if (qobject_cast<AbstractDebBasedQt4MaemoTarget *>(target()))
        m_installer = new MaemoDebianPackageInstaller(this);
    else
        m_installer = new MaemoRpmPackageInstaller(this);
    connect(m_installer, SIGNAL(stdout(QString)),
        SLOT(handleRemoteStdout(QString)));
    connect(m_installer, SIGNAL(stderr(QString)),
        SLOT(handleRemoteStderr(QString)));
    connect(m_installer, SIGNAL(finished(QString)),
        SLOT(handleInstallationFinished(QString)));

    m_copyFacility = new MaemoRemoteCopyFacility(this);
    connect(m_copyFacility, SIGNAL(stdout(QString)),
        SLOT(handleRemoteStdout(QString)));
    connect(m_copyFacility, SIGNAL(stderr(QString)),
        SLOT(handleRemoteStderr(QString)));
    connect(m_copyFacility, SIGNAL(progress(QString)),
        SLOT(handleProgressReport(QString)));
    connect(m_copyFacility, SIGNAL(fileCopied(MaemoDeployable)),
        SLOT(handleFileCopied(MaemoDeployable)));
    connect(m_copyFacility, SIGNAL(finished(QString)),
        SLOT(handleCopyingFinished(QString)));

    connect(maemotarget()->deviceConfigurationsModel(), SIGNAL(updated()),
        SLOT(handleDeviceConfigurationsUpdated()));
}

bool MaemoDeployStep::init()
{
    return true;
}

void MaemoDeployStep::run(QFutureInterface<bool> &fi)
{
    // Move to GUI thread.
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

const AbstractMaemoPackageCreationStep *MaemoDeployStep::packagingStep() const
{
    return MaemoGlobal::earlierBuildStep<AbstractMaemoPackageCreationStep>(maemoDeployConfig());
}

void MaemoDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    m_hasError = true;
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
    case Connecting:
        m_connection->disconnectFromHost();
        setState(Inactive);
        break;
    case Installing:
        m_installer->cancelInstallation();
        setState(Inactive);
        break;
    case Copying:
        m_copyFacility->cancel();
        setState(Inactive);
        break;
    case Uploading:
        m_uploader->cancelUpload();
        setState(Inactive);
        break;
    case Mounting:
    case Unmounting:
        break; // Nothing to do here.
    default:
        qFatal("Missing switch case in %s.", Q_FUNC_INFO);
    }
}

QString MaemoDeployStep::uploadDir() const
{
    return MaemoGlobal::homeDirOnDevice(m_connection->connectionParameters().userName);
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

    Q_ASSERT(!m_needsInstall);
    Q_ASSERT(m_filesToCopy.isEmpty());
    m_hasError = false;
    const AbstractMaemoPackageCreationStep * const pStep = packagingStep();
    const QString hostName = m_cachedDeviceConfig->sshParameters().host;
    if (pStep) {
        const MaemoDeployable d(pStep->packageFilePath(), QString());
        if (currentlyNeedsDeployment(hostName, d))
            m_needsInstall = true;
    } else {
        const QSharedPointer<MaemoDeployables> deployables
            = maemoDeployConfig()->deployables();
        const int deployableCount = deployables->deployableCount();
        for (int i = 0; i < deployableCount; ++i) {
            const MaemoDeployable &d = deployables->deployableAt(i);
            if (currentlyNeedsDeployment(hostName, d))
                m_filesToCopy << d;
        }
    }

    if (m_needsInstall || !m_filesToCopy.isEmpty()) {
        if (m_cachedDeviceConfig->type() == MaemoDeviceConfig::Emulator
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

void MaemoDeployStep::handleUploadFinished(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << Uploading << StopRequested);
    if (m_state == StopRequested)
        return;

    if (!errorMsg.isEmpty()) {
        raiseError(errorMsg);
        setState(Inactive);
    } else {
        writeOutput(tr("Successfully uploaded package file."));
        const QString remoteFilePath = uploadDir() + QLatin1Char('/')
            + QFileInfo(packagingStep()->packageFilePath()).fileName();
        runPackageInstaller(remoteFilePath);
    }
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
            setState(Copying);
            m_copyFacility->copyFiles(m_connection, m_filesToCopy,
                deployMountPoint());
        }
        break;
    case StopRequested:
        m_mounter->tearDownMounts();
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::handleUnmounted()
{
    ASSERT_STATE(QList<State>() << Unmounting << StopRequested << Inactive);

    switch (m_state) {
    case StopRequested:
        setState(Inactive);
        break;
    case Unmounting:
        setDeploymentFinished();
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::handleMountError(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << Mounting << Unmounting << StopRequested
        << Inactive);

    switch (m_state) {
    case Mounting:
    case Unmounting:
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
    ASSERT_STATE(QList<State>() << Mounting << Unmounting << StopRequested
        << Inactive);

    switch (m_state) {
    case Mounting:
    case Unmounting:
    case StopRequested:
        writeOutput(output, ErrorOutput);
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::mount()
{
    ASSERT_STATE(Connecting);
    setState(Mounting);

    Q_ASSERT(m_needsInstall || !m_filesToCopy.isEmpty());
    QList<MaemoMountSpecification> mountSpecs;
    if (m_needsInstall) {
        const QString localDir
            = QFileInfo(packagingStep()->packageFilePath()).absolutePath();
        mountSpecs << MaemoMountSpecification(localDir, deployMountPoint());
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
            mountSpecs << mountSpec;
            drivesToMount[index] = true;
        }
#else
        mountSpecs << MaemoMountSpecification(QLatin1String("/"),
            deployMountPoint());
#endif
    }
    m_mounter->setupMounts(m_connection, mountSpecs, freePorts(),
        qt4BuildConfiguration());
}

void MaemoDeployStep::upload()
{
    writeOutput(tr(""));
    setState(Uploading);
    const QString localFilePath = packagingStep()->packageFilePath();
    const QString fileName = QFileInfo(localFilePath).fileName();
    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + fileName;
    m_uploader->uploadPackage(m_connection, localFilePath, remoteFilePath);
}

void MaemoDeployStep::connectToDevice()
{
    ASSERT_STATE(QList<State>() << Inactive);
    setState(Connecting);

    m_connection = SshConnectionManager::instance().acquireConnection(m_cachedDeviceConfig->sshParameters());
    const bool canReUse = m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == m_cachedDeviceConfig->sshParameters();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)), this,
        SLOT(handleConnectionFailure()));
    if (canReUse) {
        handleConnected();
    } else {
        writeOutput(tr("Connecting to device..."));
        m_connection->connectToHost();
    }
}

void MaemoDeployStep::handleConnected()
{
    ASSERT_STATE(QList<State>() << Connecting << StopRequested);

    if (m_state == Connecting) {
        if (maemotarget()->allowsRemoteMounts())
            mount();
        else
            upload();
    }
}

void MaemoDeployStep::runPackageInstaller(const QString &packageFilePath)
{
    ASSERT_STATE(QList<State>() << Mounting << Uploading);
    setState(Installing);

    writeOutput(tr("Installing package to device..."));
    m_installer->installPackage(m_connection, packageFilePath,
        m_state == Uploading);
}

void MaemoDeployStep::handleProgressReport(const QString &progressMsg)
{
    ASSERT_STATE(QList<State>() << Mounting << Unmounting << Uploading
        << Copying << StopRequested << Inactive);

    switch (m_state) {
    case Mounting:
    case Unmounting:
    case Uploading:
    case Copying:
    case StopRequested:
        writeOutput(progressMsg);
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::handleFileCopied(const MaemoDeployable &deployable)
{
    setDeployed(m_connection->connectionParameters().host, deployable);
}

void MaemoDeployStep::handleCopyingFinished(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << Copying << StopRequested << Inactive);

    switch (m_state) {
    case Copying:
        if (!errorMsg.isEmpty())
            raiseError(errorMsg);
        else
            writeOutput(tr("All files copied."));
        setState(Unmounting);
        m_mounter->tearDownMounts();
        break;
    case StopRequested:
        m_mounter->tearDownMounts();
        break;
    case Inactive:
    default:
        break;
    }
}

QString MaemoDeployStep::deployMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(m_cachedDeviceConfig->sshParameters().userName)
        + QLatin1String("/deployMountPoint_") + target()->project()->displayName();
}

const MaemoToolChain *MaemoDeployStep::toolChain() const
{
    return static_cast<MaemoToolChain *>(qt4BuildConfiguration()->toolChain());
}

const AbstractQt4MaemoTarget *MaemoDeployStep::maemotarget() const
{
    return static_cast<AbstractQt4MaemoTarget *>(qt4BuildConfiguration()->target());
}

void MaemoDeployStep::handleInstallationFinished(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << Installing << StopRequested << Inactive);

    switch (m_state) {
    case Installing:
        if (errorMsg.isEmpty()) {
            m_needsInstall = false;
            setDeployed(m_connection->connectionParameters().host,
                MaemoDeployable(packagingStep()->packageFilePath(), QString()));
            writeOutput(tr("Package installed."));
        } else {
            raiseError(errorMsg);
        }
        if (maemotarget()->allowsRemoteMounts()) {
            setState(Unmounting);
            m_mounter->tearDownMounts();
        } else {
            setDeploymentFinished();
        }
        break;
    case StopRequested:
        if (maemotarget()->allowsRemoteMounts()) {
            setState(Unmounting);
            m_mounter->tearDownMounts();
        } else {
            setDeploymentFinished();
        }
        break;
    case Inactive:
    default:
        break;
    }
}

void MaemoDeployStep::setDeploymentFinished()
{
    if (m_hasError)
        writeOutput(tr("Deployment failed."), ErrorMessageOutput);
    else
        writeOutput(tr("Deployment finished."));
    setState(Inactive);
}

void MaemoDeployStep::setState(State newState)
{
    if (newState == m_state)
        return;
    m_state = newState;
    if (m_state == Inactive) {
        m_needsInstall = false;
        m_filesToCopy.clear();
        if (m_connection) {
            disconnect(m_connection.data(), 0, this, 0);
            SshConnectionManager::instance().releaseConnection(m_connection);
        }
        emit done();
    }
}

void MaemoDeployStep::handleRemoteStdout(const QString &output)
{
    ASSERT_STATE(QList<State>() << Installing << Copying << StopRequested);

    switch (m_state) {
    case Installing:
    case Copying:
    case StopRequested:
        writeOutput(output, NormalOutput);
        break;
    default:
        break;
    }
}

void MaemoDeployStep::handleRemoteStderr(const QString &output)
{
    ASSERT_STATE(QList<State>() << Installing << Copying << StopRequested);

    switch (m_state) {
    case Installing:
    case Copying:
    case StopRequested:
        writeOutput(output, ErrorOutput);
        break;
    default:
        break;
    }
}

MaemoPortList MaemoDeployStep::freePorts() const
{
    const Qt4BuildConfiguration * const qt4bc = qt4BuildConfiguration();
    const MaemoDeviceConfig::ConstPtr &devConf
        = m_cachedDeviceConfig ? m_cachedDeviceConfig : m_deviceConfig;
    if (!devConf)
        return MaemoPortList();
    if (devConf->type() == MaemoDeviceConfig::Emulator && qt4bc) {
        MaemoQemuRuntime rt;
        const int id = qt4bc->qtVersion()->uniqueId();
        if (MaemoQemuManager::instance().runtimeForQtVersion(id, &rt))
            return rt.m_freePorts;
    }
    return devConf->freePorts();
}

const Qt4BuildConfiguration *MaemoDeployStep::qt4BuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(buildConfiguration());
}

Qt4MaemoDeployConfiguration *MaemoDeployStep::maemoDeployConfig() const
{
    return qobject_cast<Qt4MaemoDeployConfiguration *>(deployConfiguration());
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
