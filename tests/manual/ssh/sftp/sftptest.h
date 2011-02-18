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

#ifndef SFTPTEST_H
#define SFTPTEST_H

#include "parameters.h"

#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

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
    void handleJobFinished(Core::SftpJobId job, const QString &error);
    void handleChannelClosed();

private:
    typedef QHash<Core::SftpJobId, QString> JobMap;
    typedef QSharedPointer<QFile> FilePtr;
    enum State { Inactive, Connecting, InitializingChannel, UploadingSmall,
        DownloadingSmall, RemovingSmall, UploadingBig, DownloadingBig,
        RemovingBig, ChannelClosing, Disconnecting
    };

    void removeFile(const FilePtr &filePtr, bool remoteToo);
    void removeFiles(bool remoteToo);
    QString cmpFileName(const QString &localFileName) const;
    QString remoteFilePath(const QString &localFileName) const;
    void earlyDisconnectFromHost();
    bool handleJobFinished(Core::SftpJobId job, JobMap &jobMap,
        const QString &error, const char *activity);
    bool handleBigJobFinished(Core::SftpJobId job, Core::SftpJobId expectedJob,
        const QString &error, const char *activity);
    bool compareFiles(QFile *orig, QFile *copy);

    const Parameters m_parameters;
    State m_state;
    bool m_error;
    Core::SshConnection::Ptr m_connection;
    Core::SftpChannel::Ptr m_channel;
    QList<FilePtr> m_localSmallFiles;
    JobMap m_smallFilesUploadJobs;
    JobMap m_smallFilesDownloadJobs;
    JobMap m_smallFilesRemovalJobs;
    FilePtr m_localBigFile;
    Core::SftpJobId m_bigFileUploadJob;
    Core::SftpJobId m_bigFileDownloadJob;
    Core::SftpJobId m_bigFileRemovalJob;
    QElapsedTimer m_bigJobTimer;
};


#endif // SFTPTEST_H
