/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SFTPTEST_H
#define SFTPTEST_H

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

private slots:
    void handleConnected();
    void handleError();
    void handleDisconnected();
    void handleChannelInitialized();
    void handleChannelInitializationFailure(const QString &reason);
    void handleJobFinished(QSsh::SftpJobId job, const QString &error);
    void handleFileInfo(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo> &fileInfoList);
    void handleChannelClosed();

private:
    typedef QHash<QSsh::SftpJobId, QString> JobMap;
    typedef QSharedPointer<QFile> FilePtr;
    enum State {
        Inactive, Connecting, InitializingChannel, UploadingSmall, DownloadingSmall,
        RemovingSmall, UploadingBig, DownloadingBig, RemovingBig, CreatingDir,
        CheckingDirAttributes, CheckingDirContents, RemovingDir, ChannelClosing, Disconnecting
    };

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


#endif // SFTPTEST_H
