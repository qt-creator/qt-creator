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

#include "parameters.h"

#include <ssh/sftpchannel.h>
#include <ssh/sshconnection.h>

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QFile);

class SftpTest : public QObject
{
    Q_OBJECT
public:
    SftpTest(const Parameters &params);
    ~SftpTest();
    void run();

private:
    typedef QHash<QSsh::SftpJobId, QString> JobMap;
    typedef QSharedPointer<QFile> FilePtr;
    enum State {
        Inactive, Connecting, InitializingChannel, UploadingSmall, DownloadingSmall,
        RemovingSmall, UploadingBig, DownloadingBig, RemovingBig, CreatingDir,
        CheckingDirAttributes, CheckingDirContents, RemovingDir, ChannelClosing, Disconnecting
    };

    void handleConnected();
    void handleError();
    void handleDisconnected();
    void handleChannelInitialized();
    void handleChannelInitializationFailure(const QString &reason);
    void handleJobFinished(QSsh::SftpJobId job, const QString &error);
    void handleFileInfo(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo> &fileInfoList);
    void handleChannelClosed();

    void removeFile(const FilePtr &filePtr, bool remoteToo);
    void removeFiles(bool remoteToo);
    QString cmpFileName(const QString &localFileName) const;
    QString remoteFilePath(const QString &localFileName) const;
    void earlyDisconnectFromHost();
    bool checkJobId(QSsh::SftpJobId job, QSsh::SftpJobId expectedJob, const char *activity);
    bool handleJobFinished(QSsh::SftpJobId job, JobMap &jobMap,
        const QString &error, const char *activity);
    bool handleJobFinished(QSsh::SftpJobId job, QSsh::SftpJobId expectedJob, const QString &error,
        const char *activity);
    bool handleBigJobFinished(QSsh::SftpJobId job, QSsh::SftpJobId expectedJob,
        const QString &error, const char *activity);
    bool compareFiles(QFile *orig, QFile *copy);

    const Parameters m_parameters;
    State m_state;
    bool m_error;
    QSsh::SshConnection *m_connection;
    QSsh::SftpChannel::Ptr m_channel;
    QList<FilePtr> m_localSmallFiles;
    JobMap m_smallFilesUploadJobs;
    JobMap m_smallFilesDownloadJobs;
    JobMap m_smallFilesRemovalJobs;
    FilePtr m_localBigFile;
    QSsh::SftpJobId m_bigFileUploadJob;
    QSsh::SftpJobId m_bigFileDownloadJob;
    QSsh::SftpJobId m_bigFileRemovalJob;
    QSsh::SftpJobId m_mkdirJob;
    QSsh::SftpJobId m_statDirJob;
    QSsh::SftpJobId m_lsDirJob;
    QSsh::SftpJobId m_rmDirJob;
    QElapsedTimer m_bigJobTimer;
    QString m_remoteDirPath;
    QSsh::SftpFileInfo m_dirInfo;
    QList<QSsh::SftpFileInfo> m_dirContents;
};
