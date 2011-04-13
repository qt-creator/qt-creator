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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SFTPTEST_H
#define SFTPTEST_H

#include "parameters.h"

#include <utils/ssh/sftpchannel.h>
#include <utils/ssh/sshconnection.h>

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
    void handleJobFinished(Utils::SftpJobId job, const QString &error);
    void handleChannelClosed();

private:
    typedef QHash<Utils::SftpJobId, QString> JobMap;
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
    bool handleJobFinished(Utils::SftpJobId job, JobMap &jobMap,
        const QString &error, const char *activity);
    bool handleBigJobFinished(Utils::SftpJobId job, Utils::SftpJobId expectedJob,
        const QString &error, const char *activity);
    bool compareFiles(QFile *orig, QFile *copy);

    const Parameters m_parameters;
    State m_state;
    bool m_error;
    Utils::SshConnection::Ptr m_connection;
    Utils::SftpChannel::Ptr m_channel;
    QList<FilePtr> m_localSmallFiles;
    JobMap m_smallFilesUploadJobs;
    JobMap m_smallFilesDownloadJobs;
    JobMap m_smallFilesRemovalJobs;
    FilePtr m_localBigFile;
    Utils::SftpJobId m_bigFileUploadJob;
    Utils::SftpJobId m_bigFileDownloadJob;
    Utils::SftpJobId m_bigFileRemovalJob;
    QElapsedTimer m_bigJobTimer;
};


#endif // SFTPTEST_H
