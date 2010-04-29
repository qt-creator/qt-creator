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

#include "sshconnection.h"

#include "ne7sshobject.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>

#include <ne7ssh.h>

#include <exception>

namespace Core {

namespace {

class GenericSshConnection
{
    Q_DECLARE_TR_FUNCTIONS(GenericSshConnection)
public:
    GenericSshConnection(const SshServerInfo &server)
        : ssh(Internal::Ne7SshObject::instance()->get()),
        m_server(server),
        m_channel(-1)
    { }

    ~GenericSshConnection()
    {
        quit();
    }

    bool start(bool shell)
    {
        Q_ASSERT(m_channel == -1);

        try {
            const QString *authString;
            int (ne7ssh::*connFunc)(const char *, int, const char *,
                                    const char *, bool, int);
            if (m_server.authType == SshServerInfo::AuthByPwd) {
                authString = &m_server.pwd;
                connFunc = &ne7ssh::connectWithPassword;
            } else {
                authString = &m_server.privateKeyFile;
                connFunc = &ne7ssh::connectWithKey;
            }
            m_channel = (ssh.data()->*connFunc)(m_server.host.toLatin1(),
                            m_server.port, m_server.uname.toAscii(),
                            authString->toLatin1(), shell, m_server.timeout);
            if (m_channel == -1) {
                setError(tr("Could not connect to host."), false);
                return false;
            }
        } catch (const std::exception &e) {
        // Should in theory not be necessary, but Net7 leaks Botan exceptions.
            setError(tr("Error in cryptography backend: %1")
                     .arg(QLatin1String(e.what())), false);
            return false;
    }

        return true;
    }

    void quit()
    {
        const int channel = m_channel;
        if (channel != -1) {
            m_channel = -1;
            if (!ssh->close(channel))
                qWarning("%s: close() failed.", Q_FUNC_INFO);
        }
    }

    bool hasError() const { return !m_error.isEmpty(); }
    QString error() const { return m_error; }
    int channel() const { return m_channel; }
    QString lastNe7Error() { return ssh->errors()->pop(channel()); }
    const SshServerInfo &server() { return m_server; }

    void setError(const QString error, bool appendNe7ErrMsg)
    {
        m_error = error;
        if (appendNe7ErrMsg)
            m_error += QLatin1String(": ") + lastNe7Error();
    }

    QSharedPointer<ne7ssh> ssh;
private:
    const SshServerInfo m_server;
    QString m_error;
    int m_channel;
};


char *alloc(size_t n)
{
    return new char[n];
}

} // anonymous namespace

namespace Internal {

struct InteractiveSshConnectionPrivate
{
    InteractiveSshConnectionPrivate(const SshServerInfo &server)
        : conn(server), outputReader(0) {}

    GenericSshConnection conn;
    ConnectionOutputReader *outputReader;
};

struct NonInteractiveSshConnectionPrivate
{
    NonInteractiveSshConnectionPrivate(const SshServerInfo &server)
        : conn(server) {}

    GenericSshConnection conn;
    Ne7SftpSubsystem sftp;
};

class ConnectionOutputReader : public QThread
{
public:
    ConnectionOutputReader(InteractiveSshConnection *parent)
        : QThread(parent), m_conn(parent), m_stopRequested(false)
    {}

    ~ConnectionOutputReader()
    {
        stop();
        wait();
    }

    // TODO: Use a wakeup mechanism here as soon as we no longer poll for output
    // from Net7.
    void stop()
    {
        m_stopRequested = true;
    }

private:
    virtual void run()
    {
        while (!m_stopRequested) {
            const int channel = m_conn->d->conn.channel();
            if (channel != -1) {
                QScopedPointer<char, QScopedPointerArrayDeleter<char> >
                    output(m_conn->d->conn.ssh->readAndReset(channel, alloc));
                if (output)
                    emit m_conn->remoteOutput(QByteArray(output.data()));
            }
            usleep(100000); // TODO: Hack Net7 to enable wait() functionality.
        }
    }

    InteractiveSshConnection *m_conn;
    bool m_stopRequested;
};

} // namespace Internal


InteractiveSshConnection::InteractiveSshConnection(const SshServerInfo &server)
    : d(new Internal::InteractiveSshConnectionPrivate(server))
{
    d->outputReader = new Internal::ConnectionOutputReader(this);
}

InteractiveSshConnection::~InteractiveSshConnection()
{
    d->conn.ssh->send("exit\n", d->conn.channel());
    quit();
    delete d;
}

bool InteractiveSshConnection::start()
{
    if (!d->conn.start(true))
        return false;

    d->outputReader->start();
    return true;
}

bool InteractiveSshConnection::sendInput(const QByteArray &input)
{
    if (!d->conn.ssh->send(input.data(), d->conn.channel())) {
        d->conn.setError(tr("Error sending input"), true);
        return false;
    }
    return true;
}

void InteractiveSshConnection::quit()
{
    d->outputReader->stop();
    d->conn.quit();
}

InteractiveSshConnection::Ptr InteractiveSshConnection::create(const SshServerInfo &server)
{
    return Ptr(new InteractiveSshConnection(server));
}

bool InteractiveSshConnection::hasError() const
{
    return d->conn.hasError();
}

QString InteractiveSshConnection::error() const
{
    return d->conn.error();
}


namespace {

class FileMgr
{
public:
    FileMgr(const QString &filePath, const char *mode)
        : m_file(fopen(filePath.toLatin1().data(), mode)) {}
    ~FileMgr() { if (m_file) fclose(m_file); }
    FILE *file() const { return m_file; }
private:
    FILE * const m_file;
};

} // Anonymous namespace

SftpConnection::SftpConnection(const SshServerInfo &server)
    : d(new Internal::NonInteractiveSshConnectionPrivate(server))
{ }

SftpConnection::~SftpConnection()
{
    quit();
    delete d;
}

bool SftpConnection::start()
{
    if (!d->conn.start(false))
        return false;
    if (!d->conn.ssh->initSftp(d->sftp, d->conn.channel())
        || !d->sftp.setTimeout(d->conn.server().timeout)) {
        d->conn.setError(tr("Error setting up SFTP subsystem"), true);
        return false;
    }
    return true;
}

bool SftpConnection::transferFiles(const QList<SftpTransferInfo> &transferList)
{
    for (int i = 0; i < transferList.count(); ++i) {
        const SftpTransferInfo &transfer = transferList.at(i);
        bool success;
        if (transfer.type == SftpTransferInfo::Upload) {
            success = upload(transfer.localFilePath, transfer.remoteFilePath);
        } else {
            success = download(transfer.remoteFilePath, transfer.localFilePath);
        }
        if (!success)
            return false;
    }

    return true;
}

bool SftpConnection::upload(const QString &localFilePath,
                            const QByteArray &remoteFilePath)
{
    FileMgr fileMgr(localFilePath, "rb");
    if (!fileMgr.file()) {
        d->conn.setError(tr("Could not open file '%1'").arg(localFilePath),
                          false);
        return false;
    }

    if (!d->sftp.put(fileMgr.file(), remoteFilePath.data())) {
        d->conn.setError(tr("Could not uplodad file '%1'")
                         .arg(localFilePath), true);
        return false;
    }

    emit fileCopied(localFilePath);
    return true;
}

bool SftpConnection::download(const QByteArray &remoteFilePath,
                              const QString &localFilePath)
{
    FileMgr fileMgr(localFilePath, "wb");
    if (!fileMgr.file()) {
        d->conn.setError(tr("Could not open file '%1'").arg(localFilePath),
                          false);
        return false;
    }

    if (!d->sftp.get(remoteFilePath.data(), fileMgr.file())) {
        d->conn.setError(tr("Could not copy remote file '%1' to local file '%2'")
                          .arg(remoteFilePath, localFilePath), false);
        return false;
    }

    emit fileCopied(remoteFilePath);
    return true;
}

bool SftpConnection::createRemoteDir(const QByteArray &remoteDir)
{
    if (!d->sftp.mkdir(remoteDir.data())) {
        d->conn.setError(tr("Could not create remote directory"), true);
        return false;
    }
    return true;
}

bool SftpConnection::removeRemoteDir(const QByteArray &remoteDir)
{
    if (!d->sftp.rmdir(remoteDir.data())) {
        d->conn.setError(tr("Could not remove remote directory"), true);
        return false;
    }
    return true;
}

QByteArray SftpConnection::listRemoteDirContents(const QByteArray &remoteDir,
                                                 bool withAttributes, bool &ok)
{
    const char * const buffer = d->sftp.ls(remoteDir.data(), withAttributes);
    if (!buffer) {
        d->conn.setError(tr("Could not get remote directory contents"), true);
        ok = false;
        return QByteArray();
    }
    ok = true;
    return QByteArray(buffer);
}

bool SftpConnection::removeRemoteFile(const QByteArray &remoteFile)
{
    if (!d->sftp.rm(remoteFile.data())) {
        d->conn.setError(tr("Could not remove remote file"), true);
        return false;
    }
    return true;
}

bool SftpConnection::changeRemoteWorkingDir(const QByteArray &newRemoteDir)
{
    if (!d->sftp.cd(newRemoteDir.data())) {
        d->conn.setError(tr("Could not change remote working directory"), true);
        return false;
    }
    return true;
}

void SftpConnection::quit()
{
    d->conn.quit();
}

bool SftpConnection::hasError() const
{
    return d->conn.hasError();
}

QString SftpConnection::error() const
{
    return d->conn.error();
}

SftpConnection::Ptr SftpConnection::create(const SshServerInfo &server)
{
    return Ptr(new SftpConnection(server));
}

} // namespace Core
