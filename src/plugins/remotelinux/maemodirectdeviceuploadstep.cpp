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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemodirectdeviceuploadstep.h"

#include "deployablefile.h"
#include "deploymentinfo.h"
#include "maemoglobal.h"
#include "qt4maemodeployconfiguration.h"

#include <utils/ssh/sftpchannel.h>
#include <utils/ssh/sshremoteprocess.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#define ASSERT_BASE_STATE(state) ASSERT_STATE_GENERIC(BaseState, state, baseState())
#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(ExtendedState, state, m_extendedState)

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

MaemoDirectDeviceUploadStep::MaemoDirectDeviceUploadStep(BuildStepList *parent)
    : AbstractMaemoDeployStep(parent, Id)
{
    ctor();
}

MaemoDirectDeviceUploadStep::MaemoDirectDeviceUploadStep(BuildStepList *parent,
    MaemoDirectDeviceUploadStep *other)
    : AbstractMaemoDeployStep(parent, other)
{
    ctor();
}

MaemoDirectDeviceUploadStep::~MaemoDirectDeviceUploadStep() {}


void MaemoDirectDeviceUploadStep::ctor()
{
    setDefaultDisplayName(displayName());
    m_extendedState = Inactive;
}

bool MaemoDirectDeviceUploadStep::isDeploymentPossibleInternal(QString &whyNot) const
{
    Q_UNUSED(whyNot);
    return true;
}

bool MaemoDirectDeviceUploadStep::isDeploymentNeeded(const QString &hostName) const
{
    m_filesToUpload.clear();
    const QSharedPointer<DeploymentInfo> deploymentInfo
        = maemoDeployConfig()->deploymentInfo();
    const int deployableCount = deploymentInfo->deployableCount();
    for (int i = 0; i < deployableCount; ++i)
        checkDeploymentNeeded(hostName, deploymentInfo->deployableAt(i));
    return !m_filesToUpload.isEmpty();
}

void MaemoDirectDeviceUploadStep::checkDeploymentNeeded(const QString &hostName,
    const DeployableFile &deployable) const
{
    QFileInfo fileInfo(deployable.localFilePath);
    if (fileInfo.isDir()) {
        const QStringList files = QDir(deployable.localFilePath)
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (files.isEmpty() && currentlyNeedsDeployment(hostName, deployable))
            m_filesToUpload << deployable;
        foreach (const QString &fileName, files) {
            const QString localFilePath = deployable.localFilePath
                + QLatin1Char('/') + fileName;
            const QString remoteDir = deployable.remoteDir + QLatin1Char('/')
                + fileInfo.fileName();
            checkDeploymentNeeded(hostName,
                DeployableFile(localFilePath, remoteDir));
        }
    } else if (currentlyNeedsDeployment(hostName, deployable)) {
        m_filesToUpload << deployable;
    }
}


void MaemoDirectDeviceUploadStep::startInternal()
{
    Q_ASSERT(m_extendedState == Inactive);

    m_uploader = connection()->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()),
        SLOT(handleSftpInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)),
        SLOT(handleSftpInitializationFailed(QString)));
    m_uploader->initialize();
    m_extendedState = InitializingSftp;
}

void MaemoDirectDeviceUploadStep::handleSftpInitializationFailed(const QString &errorMessage)
{
    ASSERT_STATE(QList<ExtendedState>() << Inactive << InitializingSftp);

    if (m_extendedState == InitializingSftp) {
        raiseError(tr("SFTP initialization failed: %1").arg(errorMessage));
        setFinished();
    }
}

void MaemoDirectDeviceUploadStep::handleSftpInitialized()
{
    ASSERT_STATE(QList<ExtendedState>() << Inactive << InitializingSftp);
    if (m_extendedState == InitializingSftp) {
        Q_ASSERT(!m_filesToUpload.isEmpty());
        connect(m_uploader.data(), SIGNAL(finished(Utils::SftpJobId, QString)),
            SLOT(handleUploadFinished(Utils::SftpJobId,QString)));
        uploadNextFile();
    }
}

void MaemoDirectDeviceUploadStep::uploadNextFile()
{
    if (m_filesToUpload.isEmpty()) {
        writeOutput(tr("All files successfully deployed."));
        setFinished();
        return;
    }

    const DeployableFile &d = m_filesToUpload.first();
    QString dirToCreate = d.remoteDir;
    QFileInfo fi(d.localFilePath);
    if (fi.isDir())
        dirToCreate += QLatin1Char('/') + fi.fileName();
    const QByteArray command = "mkdir -p " + dirToCreate.toUtf8();
    m_mkdirProc = connection()->createRemoteProcess(command);
    connect(m_mkdirProc.data(), SIGNAL(closed(int)),
        SLOT(handleMkdirFinished(int)));
    // TODO: Connect stderr.
    writeOutput(tr("Uploading file '%1'...")
        .arg(QDir::toNativeSeparators(d.localFilePath)));
    m_mkdirProc->start();
    m_extendedState = Uploading;
}

void MaemoDirectDeviceUploadStep::handleMkdirFinished(int exitStatus)
{
    ASSERT_STATE(QList<ExtendedState>() << Inactive << Uploading);
    if (m_extendedState == Inactive)
        return;

    const DeployableFile &d = m_filesToUpload.first();
    QFileInfo fi(d.localFilePath);
    const QString nativePath = QDir::toNativeSeparators(d.localFilePath);
    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_mkdirProc->exitCode() != 0) {
        raiseError(tr("Failed to upload file '%1'.").arg(nativePath));
        setFinished();
    } else if (fi.isDir()) {
        setDeployed(deviceConfiguration()->sshParameters().host, d);
        m_filesToUpload.removeFirst();
        uploadNextFile();
    } else {
        const SftpJobId job = m_uploader->uploadFile(d.localFilePath,
            d.remoteDir + QLatin1Char('/')  + fi.fileName(),
            SftpOverwriteExisting);
        if (job == SftpInvalidJob) {
            raiseError(tr("Failed to upload file '%1': "
                "Could not open for reading.").arg(nativePath));
            setFinished();
        }
    }
}

void MaemoDirectDeviceUploadStep::handleUploadFinished(Utils::SftpJobId jobId,
    const QString &errorMsg)
{
    Q_UNUSED(jobId);

    ASSERT_STATE(QList<ExtendedState>() << Inactive << Uploading);
    if (m_extendedState == Inactive)
        return;

    const DeployableFile d = m_filesToUpload.takeFirst();
    if (!errorMsg.isEmpty()) {
        raiseError(tr("Upload of file '%1' failed: %2")
            .arg(QDir::toNativeSeparators(d.localFilePath), errorMsg));
        setFinished();
    } else {
        setDeployed(connection()->connectionParameters().host, d);
        uploadNextFile();
    }
}

void MaemoDirectDeviceUploadStep::stopInternal()
{
    ASSERT_BASE_STATE(StopRequested);
    ASSERT_STATE(QList<ExtendedState>() << InitializingSftp << Uploading);

    setFinished();
}

void MaemoDirectDeviceUploadStep::setFinished()
{
    m_extendedState = Inactive;
    if (m_mkdirProc) {
        disconnect(m_mkdirProc.data(), 0, this, 0);
    }
    if (m_uploader) {
        disconnect(m_uploader.data(), 0, this, 0);
        m_uploader->closeChannel();
    }
    setDeploymentFinished();
}

const QString MaemoDirectDeviceUploadStep::Id("MaemoDirectDeviceUploadStep");

QString MaemoDirectDeviceUploadStep::displayName()
{
    return tr("Upload files via SFTP");
}

} // namespace Internal
} // namespace RemoteLinux
