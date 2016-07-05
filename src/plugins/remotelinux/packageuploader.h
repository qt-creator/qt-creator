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

#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QString>

#include <ssh/sftpdefs.h>

namespace QSsh {
class SftpChannel;
class SshConnection;
}

namespace RemoteLinux {
namespace Internal {

class PackageUploader : public QObject
{
    Q_OBJECT
public:
    explicit PackageUploader(QObject *parent = 0);
    ~PackageUploader();

    // Connection has to be established already.
    void uploadPackage(QSsh::SshConnection *connection,
        const QString &localFilePath, const QString &remoteFilePath);
    void cancelUpload();

signals:
    void progress(const QString &message);
    void uploadFinished(const QString &errorMsg = QString());

private:
    enum State { InitializingSftp, Uploading, Inactive };

    void handleConnectionFailure();
    void handleSftpChannelInitialized();
    void handleSftpChannelError(const QString &error);
    void handleSftpJobFinished(QSsh::SftpJobId job, const QString &error);
    void cleanup();
    void setState(State newState);

    State m_state;
    QSsh::SshConnection *m_connection;
    QSharedPointer<QSsh::SftpChannel> m_uploader;
    QString m_localFilePath;
    QString m_remoteFilePath;
};

} // namespace Internal
} // namespace RemoteLinux
