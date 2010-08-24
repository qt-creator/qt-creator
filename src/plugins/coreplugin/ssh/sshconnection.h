/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SSHCONNECTION_H
#define SSHCONNECTION_H

#include "ssherrors.h"

#include <coreplugin/core_global.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace Core {
class SftpChannel;
class SshRemoteProcess;

namespace Internal {
class SshConnectionPrivate;
} // namespace Internal

struct CORE_EXPORT SshConnectionParameters
{
    SshConnectionParameters();

    QString host;
    QString uname;
    QString pwd;
    QString privateKeyFile;
    int timeout;
    enum AuthType { AuthByPwd, AuthByKey } authType;
    quint16 port;
};

CORE_EXPORT bool operator==(const SshConnectionParameters &p1, const SshConnectionParameters &p2);
CORE_EXPORT bool operator!=(const SshConnectionParameters &p1, const SshConnectionParameters &p2);

/*
 * This class provides an SSH connection, implementing protocol version 2.0
 * It can spawn channels for remote execution and SFTP operations (version 3).
 * It operates asynchronously (non-blocking) and is not thread-safe.
 */
class CORE_EXPORT SshConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SshConnection)
public:
    enum State { Unconnected, Connecting, Connected };
    typedef QSharedPointer<SshConnection> Ptr;

    static Ptr create();

    void connectToHost(const SshConnectionParameters &serverInfo);
    void disconnectFromHost();
    State state() const;
    SshError errorState() const;
    QString errorString() const;
    SshConnectionParameters connectionParameters() const;
    ~SshConnection();

    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SftpChannel> createSftpChannel();

signals:
    void connected();
    void disconnected();
    void dataAvailable(const QString &message);
    void error(SshError);

private:
    SshConnection();

    Internal::SshConnectionPrivate *d;
};

} // namespace Internal

#endif // SSHCONNECTION_H
