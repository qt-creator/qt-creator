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

#include "maemopackageuploader.h"

#include "maemoglobal.h"

#include <utils/ssh/sftpchannel.h>
#include <utils/ssh/sshconnection.h>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

MaemoPackageUploader::MaemoPackageUploader(QObject *parent) :
    QObject(parent), m_state(Inactive)
{
}

MaemoPackageUploader::~MaemoPackageUploader()
{
}

void MaemoPackageUploader::uploadPackage(const SshConnection::Ptr &connection,
    const QString &localFilePath, const QString &remoteFilePath)
{
    ASSERT_STATE(Inactive);
    setState(InitializingSftp);
    emit progress(tr("Preparing SFTP connection..."));

    m_localFilePath = localFilePath;
    m_remoteFilePath = remoteFilePath;
    m_connection = connection;
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)),
        SLOT(handleConnectionFailure()));
    m_uploader = m_connection->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()), this,
        SLOT(handleSftpChannelInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
        SLOT(handleSftpChannelInitializationFailed(QString)));
    connect(m_uploader.data(), SIGNAL(finished(Utils::SftpJobId, QString)),
        this, SLOT(handleSftpJobFinished(Utils::SftpJobId, QString)));
    m_uploader->initialize();
}

void MaemoPackageUploader::cancelUpload()
{
    ASSERT_STATE(QList<State>() << InitializingSftp << Uploading);

    cleanup();
}

void MaemoPackageUploader::handleConnectionFailure()
{
    if (m_state == Inactive)
        return;

    const QString errorMsg = m_connection->errorString();
    setState(Inactive);
    emit uploadFinished(tr("Connection failed: %1").arg(errorMsg));
}

void MaemoPackageUploader::handleSftpChannelInitializationFailed(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << InitializingSftp << Inactive);
    if (m_state == Inactive)
        return;

    setState(Inactive);
    emit uploadFinished(tr("SFTP error: %1").arg(errorMsg));
}

void MaemoPackageUploader::handleSftpChannelInitialized()
{
    ASSERT_STATE(QList<State>() << InitializingSftp << Inactive);
    if (m_state == Inactive)
        return;

    const SftpJobId job = m_uploader->uploadFile(m_localFilePath,
        m_remoteFilePath, SftpOverwriteExisting);
    if (job == SftpInvalidJob) {
        setState(Inactive);
        emit uploadFinished(tr("Package upload failed: Could not open file."));
    } else {
        emit progress("Starting upload...");
        setState(Uploading);
    }
}

void MaemoPackageUploader::handleSftpJobFinished(SftpJobId, const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << Uploading << Inactive);
    if (m_state == Inactive)
        return;

    if (!errorMsg.isEmpty())
        emit uploadFinished(tr("Failed to upload package: %2").arg(errorMsg));
    else
        emit uploadFinished();
    cleanup();
}

void MaemoPackageUploader::cleanup()
{
    m_uploader->closeChannel();
    setState(Inactive);
}

void MaemoPackageUploader::setState(State newState)
{
    if (m_state == newState)
        return;
    if (newState == Inactive) {
        if (m_uploader) {
            disconnect(m_uploader.data(), 0, this, 0);
            m_uploader.clear();
        }
        if (m_connection) {
            disconnect(m_connection.data(), 0, this, 0);
            m_connection.clear();
        }
    }
    m_state = newState;
}

} // namespace Internal
} // namespace RemoteLinux
