/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "packageuploader.h"

#include <utils/qtcassert.h>
#include <ssh/sftptransfer.h>
#include <ssh/sshconnection.h>

using namespace QSsh;

namespace RemoteLinux {
namespace Internal {

PackageUploader::PackageUploader(QObject *parent) :
    QObject(parent), m_state(Inactive), m_connection(nullptr)
{
}

PackageUploader::~PackageUploader() = default;

void PackageUploader::uploadPackage(SshConnection *connection,
    const QString &localFilePath, const QString &remoteFilePath)
{
    QTC_ASSERT(m_state == Inactive, return);

    setState(Uploading);
    emit progress(tr("Preparing SFTP connection..."));

    m_connection = connection;
    connect(m_connection, &SshConnection::errorOccurred,
            this, &PackageUploader::handleConnectionFailure);
    m_uploader = m_connection->createUpload({FileToTransfer(localFilePath, remoteFilePath)},
                                            FileTransferErrorHandling::Abort);
    connect(m_uploader.get(), &SftpTransfer::done, this, &PackageUploader::handleUploadDone);
    m_uploader->start();
}

void PackageUploader::cancelUpload()
{
    QTC_ASSERT(m_state == Uploading, return);

    setState(Inactive);
    emit uploadFinished(tr("Package upload canceled."));
}

void PackageUploader::handleConnectionFailure()
{
    if (m_state == Inactive)
        return;

    const QString errorMsg = m_connection->errorString();
    setState(Inactive);
    emit uploadFinished(tr("Connection failed: %1").arg(errorMsg));
}

void PackageUploader::handleUploadDone(const QString &errorMsg)
{
    QTC_ASSERT(m_state == Uploading, return);

    setState(Inactive);
    if (!errorMsg.isEmpty())
        emit uploadFinished(tr("Failed to upload package: %2").arg(errorMsg));
    else
        emit uploadFinished();
}

void PackageUploader::setState(State newState)
{
    if (m_state == newState)
        return;
    if (newState == Inactive) {
        if (m_uploader) {
            disconnect(m_uploader.get(), nullptr, this, nullptr);
            m_uploader->stop();
            m_uploader.release()->deleteLater();
        }
        if (m_connection) {
            disconnect(m_connection, nullptr, this, nullptr);
            m_connection = nullptr;
        }
    }
    m_state = newState;
}

} // namespace Internal
} // namespace RemoteLinux
