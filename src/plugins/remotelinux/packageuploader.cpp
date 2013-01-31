/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "packageuploader.h"

#include <utils/qtcassert.h>
#include <ssh/sftpchannel.h>
#include <ssh/sshconnection.h>

using namespace QSsh;

namespace RemoteLinux {
namespace Internal {

PackageUploader::PackageUploader(QObject *parent) :
    QObject(parent), m_state(Inactive), m_connection(0)
{
}

PackageUploader::~PackageUploader()
{
}

void PackageUploader::uploadPackage(SshConnection *connection,
    const QString &localFilePath, const QString &remoteFilePath)
{
    QTC_ASSERT(m_state == Inactive, return);

    setState(InitializingSftp);
    emit progress(tr("Preparing SFTP connection..."));

    m_localFilePath = localFilePath;
    m_remoteFilePath = remoteFilePath;
    m_connection = connection;
    connect(m_connection, SIGNAL(error(QSsh::SshError)), SLOT(handleConnectionFailure()));
    m_uploader = m_connection->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()), this,
        SLOT(handleSftpChannelInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
        SLOT(handleSftpChannelInitializationFailed(QString)));
    connect(m_uploader.data(), SIGNAL(finished(QSsh::SftpJobId,QString)),
        this, SLOT(handleSftpJobFinished(QSsh::SftpJobId,QString)));
    m_uploader->initialize();
}

void PackageUploader::cancelUpload()
{
    QTC_ASSERT(m_state == InitializingSftp || m_state == Uploading, return);

    cleanup();
}

void PackageUploader::handleConnectionFailure()
{
    if (m_state == Inactive)
        return;

    const QString errorMsg = m_connection->errorString();
    setState(Inactive);
    emit uploadFinished(tr("Connection failed: %1").arg(errorMsg));
}

void PackageUploader::handleSftpChannelInitializationFailed(const QString &errorMsg)
{
    QTC_ASSERT(m_state == InitializingSftp || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    setState(Inactive);
    emit uploadFinished(tr("SFTP error: %1").arg(errorMsg));
}

void PackageUploader::handleSftpChannelInitialized()
{
    QTC_ASSERT(m_state == InitializingSftp || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    const SftpJobId job = m_uploader->uploadFile(m_localFilePath,
        m_remoteFilePath, SftpOverwriteExisting);
    if (job == SftpInvalidJob) {
        setState(Inactive);
        emit uploadFinished(tr("Package upload failed: Could not open file."));
    } else {
        emit progress(tr("Starting upload..."));
        setState(Uploading);
    }
}

void PackageUploader::handleSftpJobFinished(SftpJobId, const QString &errorMsg)
{
    QTC_ASSERT(m_state == Uploading || m_state == Inactive, return);

    if (m_state == Inactive)
        return;

    if (!errorMsg.isEmpty())
        emit uploadFinished(tr("Failed to upload package: %2").arg(errorMsg));
    else
        emit uploadFinished();
    cleanup();
}

void PackageUploader::cleanup()
{
    m_uploader->closeChannel();
    setState(Inactive);
}

void PackageUploader::setState(State newState)
{
    if (m_state == newState)
        return;
    if (newState == Inactive) {
        if (m_uploader) {
            disconnect(m_uploader.data(), 0, this, 0);
            m_uploader.clear();
        }
        if (m_connection) {
            disconnect(m_connection, 0, this, 0);
            m_connection = 0;
        }
    }
    m_state = newState;
}

} // namespace Internal
} // namespace RemoteLinux
