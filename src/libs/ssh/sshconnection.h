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

#include "sftpdefs.h"
#include "ssh_global.h"

#include <QByteArray>
#include <QFlags>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QHostAddress>
#include <QUrl>

#include <memory>

namespace Utils { class FilePath; }

namespace QSsh {
class SshRemoteProcess;

enum SshHostKeyCheckingMode {
    SshHostKeyCheckingNone,
    SshHostKeyCheckingStrict,
    SshHostKeyCheckingAllowNoMatch,
};

class QSSH_EXPORT SshConnectionParameters
{
public:
    enum AuthenticationType {
        AuthenticationTypeAll,
        AuthenticationTypeSpecificKey,
    };

    SshConnectionParameters();

    QString host() const { return url.host(); }
    quint16 port() const { return url.port(); }
    QString userName() const { return url.userName(); }
    void setHost(const QString &host) { url.setHost(host); }
    void setPort(int port) { url.setPort(port); }
    void setUserName(const QString &name) { url.setUserName(name); }

    QUrl url;
    QString privateKeyFile;
    QString x11DisplayName;
    int timeout = 0; // In seconds.
    AuthenticationType authenticationType = AuthenticationTypeAll;
    SshHostKeyCheckingMode hostKeyCheckingMode = SshHostKeyCheckingAllowNoMatch;
};

QSSH_EXPORT bool operator==(const SshConnectionParameters &p1, const SshConnectionParameters &p2);
QSSH_EXPORT bool operator!=(const SshConnectionParameters &p1, const SshConnectionParameters &p2);

class QSSH_EXPORT SshConnectionInfo
{
public:
    SshConnectionInfo() = default;
    SshConnectionInfo(const QHostAddress &la, quint16 lp, const QHostAddress &pa, quint16 pp)
        : localAddress(la), localPort(lp), peerAddress(pa), peerPort(pp) {}

    bool isValid() const { return peerPort != 0; }

    QHostAddress localAddress;
    quint16 localPort = 0;
    QHostAddress peerAddress;
    quint16 peerPort = 0;
};

using SshRemoteProcessPtr = std::unique_ptr<SshRemoteProcess>;

class QSSH_EXPORT SshConnection : public QObject
{
    Q_OBJECT

public:
    enum State { Unconnected, Connecting, Connected, Disconnecting };

    explicit SshConnection(const SshConnectionParameters &serverInfo, QObject *parent = nullptr);

    void connectToHost();
    void disconnectFromHost();
    State state() const;
    QString errorString() const;
    SshConnectionParameters connectionParameters() const;
    SshConnectionInfo connectionInfo() const;
    QStringList connectionOptions(const Utils::FilePath &binary) const;
    bool sharingEnabled() const;
    ~SshConnection();

    SshRemoteProcessPtr createRemoteProcess(const QString &command);
    SshRemoteProcessPtr createRemoteShell();
    SftpTransferPtr createUpload(const FilesToTransfer &files,
                                 FileTransferErrorHandling errorHandlingMode);
    SftpTransferPtr createDownload(const FilesToTransfer &files,
                                   FileTransferErrorHandling errorHandlingMode);
    SftpSessionPtr createSftpSession();

signals:
    void connected();
    void disconnected();
    void dataAvailable(const QString &message);
    void errorOccurred();

private:
    void doConnectToHost();
    void emitError(const QString &reason);
    void emitConnected();
    void emitDisconnected();
    SftpTransferPtr setupTransfer(const FilesToTransfer &files, Internal::FileTransferType type,
                                  FileTransferErrorHandling errorHandlingMode);

    struct SshConnectionPrivate;
    SshConnectionPrivate * const d;
};

} // namespace QSsh

Q_DECLARE_METATYPE(QSsh::SshConnectionParameters::AuthenticationType)
