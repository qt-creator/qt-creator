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
#include "maemopackagecreationstep.h"
#include "maemorunconfiguration.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <QtCore/QCryptographicHash>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

using namespace Core;
using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

const QLatin1String MaemoDeployStep::Id("Qt4ProjectManager.MaemoDeployStep");


MaemoDeployStep::MaemoDeployStep(ProjectExplorer::BuildConfiguration *bc)
    : BuildStep(bc, Id)
{
    ctor();
}

MaemoDeployStep::MaemoDeployStep(ProjectExplorer::BuildConfiguration *bc,
    MaemoDeployStep *other)
    : BuildStep(bc, other), m_lastDeployed(other->m_lastDeployed)
{
    ctor();
}

MaemoDeployStep::~MaemoDeployStep() {}

void MaemoDeployStep::ctor()
{
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
    const QList<ProjectExplorer::BuildStep *> &buildSteps
        = buildConfiguration()->steps(ProjectExplorer::BuildStep::Deploy);
    for (int i = buildSteps.count() - 1; i >= 0; --i) {
        const MaemoPackageCreationStep * const pStep
                = qobject_cast<MaemoPackageCreationStep *>(buildSteps.at(i));
        if (pStep)
            return pStep;
    }
    Q_ASSERT(!"Impossible: Maemo run configuration without packaging step.");
    return 0;
}

void MaemoDeployStep::raiseError(const QString &errorString)
{
    emit addTask(Task(Task::Error, errorString, QString(), -1,
        Constants::TASK_CATEGORY_BUILDSYSTEM));
    stop();
    emit error();
}

void MaemoDeployStep::writeOutput(const QString &text,
    const QTextCharFormat &format)
{
    emit addOutput(text, format);
}

void MaemoDeployStep::stop()
{
    if (m_installer && m_installer->isRunning()) {
        disconnect(m_installer.data(), 0, this, 0);
    } else if (!m_uploadsInProgress.isEmpty() || !m_linksInProgress.isEmpty()) {
        m_uploadsInProgress.clear();
        m_linksInProgress.clear();
        m_uploader->closeChannel();
        disconnect(m_uploader.data(), 0, this, 0);
    }
    if (m_connection)
        disconnect(m_connection.data(), 0, this, 0);
    m_stopped = true;
}

QString MaemoDeployStep::uploadDir() const
{
    return homeDirOnDevice(m_connection->connectionParameters().uname);
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
    // TODO: For lib template, get info from config widget
    const RunConfiguration * const rc =
        buildConfiguration()->target()->activeRunConfiguration();
    return rc ? qobject_cast<const MaemoRunConfiguration *>(rc)->deviceConfig()
        : MaemoDeviceConfig();
}

QString MaemoDeployStep::packageFileName() const
{
    return QFileInfo(packageFilePath()).fileName();
}

QString MaemoDeployStep::packageFilePath() const
{
    return packagingStep()->packageFilePath();
}

void MaemoDeployStep::start()
{
    m_stopped = false;
    if (m_stopped)
        return;

    // TODO: Re-use if possible (disconnect + reconnect).
    if (m_connection)
        disconnect(m_connection.data(), 0, this, 0);
    m_connection = SshConnection::create();

    const MaemoDeviceConfig &devConfig = deviceConfig();
    if (!devConfig.isValid()) {
        raiseError(tr("Deployment failed: No valid device set."));
        return;
    }
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(SshError)), this,
        SLOT(handleConnectionFailure()));
    m_connection->connectToHost(devConfig.server);
}

void MaemoDeployStep::handleConnected()
{
    if (m_stopped)
        return;

    // TODO: If nothing to deploy, skip this step.
    m_uploader = m_connection->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()), this,
        SLOT(handleSftpChannelInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
        SLOT(handleSftpChannelInitializationFailed(QString)));
    connect(m_uploader.data(), SIGNAL(finished(Core::SftpJobId, QString)),
        this, SLOT(handleSftpJobFinished(Core::SftpJobId, QString)));
    m_uploader->initialize();
}

void MaemoDeployStep::handleConnectionFailure()
{
    if (m_stopped)
        return;

    raiseError(tr("Could not connect to host: %1")
        .arg(m_connection->errorString()));
}

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
        const MaemoDeployable d(packageFilePath(), uploadDir());
        if (currentlyNeedsDeployment(hostName, d)) {
            if (!deploy(MaemoDeployable(d)))
                return;
            m_needsInstall = true;
        } else {
            m_needsInstall = false;
        }
    } else {
        const MaemoDeployables * const deployables = pStep->deployables();
        const int deployableCount = deployables->deployableCount();
        for (int i = 0; i < deployableCount; ++i) {
            const MaemoDeployable &d = deployables->deployableAt(i);
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
    QByteArray linkCommand = remoteSudo().toUtf8() + " ln -sf "
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
            const QByteArray cmd = remoteSudo().toUtf8() + " dpkg -i "
                + packageFileName().toUtf8();
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

void MaemoDeployStep::handleInstallationFinished(int exitStatus)
{
    if (m_stopped)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_installer->exitCode() != 0) {
        raiseError(tr("Installing package failed."));
    } else {
        writeOutput(tr("Package installation finished."));
        emit done();
    }
}

void MaemoDeployStep::handleInstallerOutput(const QByteArray &output)
{
    writeOutput(QString::fromUtf8(output));
}

void MaemoDeployStep::handleInstallerErrorOutput(const QByteArray &output)
{
    QTextCharFormat format;
    format.setForeground(QBrush(QColor("red")));
    writeOutput(output, format);
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
