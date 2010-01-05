/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
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

#include "maemosshconnection.h"

#ifdef USE_SSH_LIB

#include "maemodeviceconfigurations.h"

#include "/opt/ne7sshModified/include/ne7ssh.h"

#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>
#include <QtCore/QStringList>

#include <cstdio>
#include <cstring>

namespace Qt4ProjectManager {
namespace Internal {
namespace {
    ne7ssh ssh;
}

MaemoSshConnection::MaemoSshConnection(const MaemoDeviceConfig &devConf,
                                       bool shell)
    : m_channel(-1), m_stopRequested(false)
{
    const QString *authString;
    int (ne7ssh::*connFunc)(const char *, int, const char *, const char *, bool, int);
    if (devConf.authentication == MaemoDeviceConfig::Password) {
        authString = &devConf.pwd;
        connFunc = &ne7ssh::connectWithPassword;
    } else {
        authString = &devConf.keyFile;
        connFunc = &ne7ssh::connectWithKey;
    }
    m_channel = (ssh.*connFunc)(devConf.host.toAscii(), devConf.port,
        devConf.uname.toAscii(), authString->toAscii(), shell, devConf.timeout);
    if (m_channel == -1)
        throw MaemoSshException(tr("Could not connect to host"));
}

MaemoSshConnection::~MaemoSshConnection()
{
    qDebug("%s", Q_FUNC_INFO);
    ssh.close(m_channel);
}

const char *MaemoSshConnection::lastError()
{
    return ssh.errors()->pop(channel());
}

void MaemoSshConnection::stop()
{
    m_stopRequested = true;
}

MaemoInteractiveSshConnection::MaemoInteractiveSshConnection(const MaemoDeviceConfig &devConf)
    : MaemoSshConnection(devConf, true), m_prompt(0)
{
    m_prompt = devConf.uname == QLatin1String("root") ? "# " : "$ ";
    if (!ssh.waitFor(channel(), m_prompt, devConf.timeout)) {
        const QString error
            = tr("Could not start remote shell: %1").arg(lastError());
        throw MaemoSshException(error);
    }
}

MaemoInteractiveSshConnection::~MaemoInteractiveSshConnection()
{
    ssh.send("exit\n", channel());
    ssh.waitFor(channel(), m_prompt, 1);
}

void MaemoInteractiveSshConnection::runCommand(const QString &command)
{
    if (!ssh.send((command + QLatin1String("\n")).toLatin1().data(),
                  channel())) {
        throw MaemoSshException(tr("Error running command: %1")
                                .arg(lastError()));
    }

    bool done;
    do {
        done = ssh.waitFor(channel(), m_prompt, 1);
        const char * const error = lastError();
        if (error)
            throw MaemoSshException(tr("SSH error: %1").arg(error));
        ssh.lock();
        const char * output = ssh.read(channel(), false);
        if (output) {
            emit remoteOutput(QLatin1String(output));
            ssh.resetInput(channel(), false);
        }
        ssh.unlock();
    } while (!done && !stopRequested());
}

MaemoInteractiveSshConnection::Ptr MaemoInteractiveSshConnection::create(const MaemoDeviceConfig &devConf)
{
    return Ptr(new MaemoInteractiveSshConnection(devConf));
}

MaemoSftpConnection::MaemoSftpConnection(const MaemoDeviceConfig &devConf)
    : MaemoSshConnection(devConf, false),
      sftp(new Ne7SftpSubsystem)
{
    if (!ssh.initSftp(*sftp, channel()) || !sftp->setTimeout(devConf.timeout))
        throw MaemoSshException(tr("Error setting up SFTP subsystem: %1")
                                .arg(lastError()));
}

MaemoSftpConnection::~MaemoSftpConnection()
{

}

void MaemoSftpConnection::transferFiles(const QStringList &filePaths,
                                        const QStringList &targetDirs)
{
    Q_ASSERT(filePaths.count() == targetDirs.count());
    for (int i = 0; i < filePaths.count(); ++i) {
        const QString &curFile = filePaths.at(i);
        QSharedPointer<FILE> filePtr(fopen(curFile.toLatin1().data(), "rb"),
                                     &std::fclose);
        if (filePtr.isNull())
            throw MaemoSshException(tr("Could not open file '%1'").arg(curFile));
        const QString &targetFile = targetDirs.at(i) % QLatin1String("/")
                                    % QFileInfo(curFile).fileName();
        if (!sftp->put(filePtr.data(), targetFile.toLatin1().data())) {
            const QString &error = tr("Could not copy local file '%1' "
                    "to remote file '%2': %3").arg(curFile, targetFile)
                    .arg(lastError());
            throw MaemoSshException(error);
        }
        emit fileCopied(curFile);
    }
}

MaemoSftpConnection::Ptr MaemoSftpConnection::create(const MaemoDeviceConfig &devConf)
{
    return Ptr(new MaemoSftpConnection(devConf));
}

} // namespace Internal
} // namespace Qt4ProjectManager

#endif
