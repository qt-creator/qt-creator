/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SSHCONNECTION_H
#define SSHCONNECTION_H

#include <coreplugin/core_global.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace Core {

namespace Internal {
    struct InteractiveSshConnectionPrivate;
    struct NonInteractiveSshConnectionPrivate;
    class ConnectionOutputReader;
}

struct CORE_EXPORT SshServerInfo
{
    QString host;
    QString uname;
    QString pwd;
    QString privateKeyFile;
    int timeout;
    enum AuthType { AuthByPwd, AuthByKey } authType;
    quint16 port;
};


class CORE_EXPORT InteractiveSshConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(InteractiveSshConnection)
    friend class Internal::ConnectionOutputReader;
public:
    typedef QSharedPointer<InteractiveSshConnection> Ptr;

    static Ptr create(const SshServerInfo &server);

    bool start();
    void quit();
    bool sendInput(const QByteArray &input); // Should normally end in newline.
    bool hasError() const;
    QString error() const;
    ~InteractiveSshConnection();

signals:
    void remoteOutput(const QByteArray &output);

private:
    InteractiveSshConnection(const SshServerInfo &server);

    struct Internal::InteractiveSshConnectionPrivate *d;
};


struct CORE_EXPORT SftpTransferInfo
{
    enum Type { Upload, Download };

    SftpTransferInfo(const QString &localFilePath,
        const QByteArray &remoteFilePath, Type type)
        : localFilePath(localFilePath),
          remoteFilePath(remoteFilePath),
          type(type)
    {
    }

    QString localFilePath;
    QByteArray remoteFilePath;
    Type type;
};

class CORE_EXPORT SftpConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SftpConnection)
public:
    typedef QSharedPointer<SftpConnection> Ptr;

    static Ptr create(const SshServerInfo &server);
    bool start();
    void quit();
    bool hasError() const;
    QString error() const;
    bool upload(const QString &localFilePath, const QByteArray &remoteFilePath);
    bool download(const QByteArray &remoteFilePath, const QString &localFilePath);
    bool transferFiles(const QList<SftpTransferInfo> &transferList);
    bool createRemoteDir(const QByteArray &remoteDir);
    bool removeRemoteDir(const QByteArray &remoteDir);
    bool removeRemoteFile(const QByteArray &remoteFile);
    bool changeRemoteWorkingDir(const QByteArray &newRemoteDir);
    QByteArray listRemoteDirContents(const QByteArray &remoteDir,
                                     bool withAttributes, bool &ok);
    ~SftpConnection();

signals:
    void fileCopied(const QString &filePath);

private:
    SftpConnection(const SshServerInfo &server);

    Internal::NonInteractiveSshConnectionPrivate *d;
};

} // namespace Core

#endif // SSHCONNECTION_H
