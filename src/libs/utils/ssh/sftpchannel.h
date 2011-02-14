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

#ifndef SFTCHANNEL_H
#define SFTCHANNEL_H

#include "sftpdefs.h"
#include "sftpincomingpacket_p.h"

#include <utils/utils_global.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace Utils {

namespace Internal {
class SftpChannelPrivate;
class SshChannelManager;
class SshSendFacility;
} // namespace Internal

/*
 * This class provides SFTP operations.
 * Objects are created via SshConnection::createSftpChannel().
 * The channel needs to be initialized with
 * a call to initialize() and is closed via closeChannel(). After closing
 * a channel, no more operations are possible. It cannot be re-opened
 * using initialize(); use SshConnection::createSftpChannel() if you need
 * a new one.
 * After the initialized() signal has been emitted, operations can be started.
 * All SFTP operations are asynchronous (non-blocking) and can be in-flight
 * simultaneously (though callers must ensure that concurrently running jobs
 * are independent of each other, e.g. they must not write to the same file).
 * Operations are identified by their job id, which is returned by
 * the respective member function. If the function can right away detect that
 * the operation cannot succeed, it returns SftpInvalidJob. If an error occurs
 * later, the finished() signal is emitted for the respective job with a
 * non-empty error string.
 * Note that directory names must not have a trailing slash.
 */
class QTCREATOR_UTILS_EXPORT SftpChannel : public QObject
{
    Q_OBJECT

    friend class Internal::SftpChannelPrivate;
    friend class Internal::SshChannelManager;
public:
    typedef QSharedPointer<SftpChannel> Ptr;

    enum State { Uninitialized, Initializing, Initialized, Closing, Closed };
    State state() const;

    void initialize();
    void closeChannel();

    SftpJobId listDirectory(const QString &dirPath);
    SftpJobId createDirectory(const QString &dirPath);
    SftpJobId removeDirectory(const QString &dirPath);
    SftpJobId removeFile(const QString &filePath);
    SftpJobId renameFileOrDirectory(const QString &oldPath,
        const QString &newPath);
    SftpJobId createFile(const QString &filePath, SftpOverwriteMode mode);
    SftpJobId uploadFile(const QString &localFilePath,
        const QString &remoteFilePath, SftpOverwriteMode mode);
    SftpJobId downloadFile(const QString &remoteFilePath,
        const QString &localFilePath, SftpOverwriteMode mode);
    SftpJobId uploadDir(const QString &localDirPath,
        const QString &remoteParentDirPath);

    ~SftpChannel();

signals:
    void initialized();
    void initializationFailed(const QString &reason);
    void closed();

    // error.isEmpty <=> finished successfully
    void finished(Utils::SftpJobId job, const QString &error = QString());

    /*
     * This signal is only emitted by the "List Directory" operation,
     * one file at a time.
     // TODO: Also emit for each file copied by uploadDir().
     */
    void dataAvailable(Utils::SftpJobId job, const QString &data);

private:
    SftpChannel(quint32 channelId, Internal::SshSendFacility &sendFacility);

    Internal::SftpChannelPrivate *d;
};

} // namespace Utils

#endif // SFTPCHANNEL_H
