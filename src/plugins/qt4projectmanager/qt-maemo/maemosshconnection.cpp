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

#include "maemodeviceconfigurations.h"

#include <ne7ssh.h>

#include <QtCore/QFileInfo>
#include <QtCore/QScopedPointer>
#include <QtCore/QStringBuilder>

#include <cstdio>
#include <cstring>

namespace Qt4ProjectManager {
namespace Internal {
namespace {
    ne7ssh ssh;

    char *alloc(size_t n)
    {
        return new char[n];
    }
}

// TODO: Which encoding to use for file names? Unicode? Latin1? ASCII?

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
    m_channel = (ssh.*connFunc)(devConf.host.toLatin1(), devConf.sshPort,
        devConf.uname.toAscii(), authString->toLatin1(), shell, devConf.timeout);
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
        QScopedPointer<char, QScopedPointerArrayDeleter<char> >
            output(ssh.readAndReset(channel(), alloc));
        if (output.data()) {
            emit remoteOutput(QString::fromUtf8(output.data()));
            if (!done)
                done = strstr(output.data(), m_prompt) != 0;
        }
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

class FileManager
{
public:
    FileManager(const QString &filePath)
        : m_file(fopen(filePath.toLatin1().data(), "rb")) {}
    ~FileManager() { if (m_file) fclose(m_file); }
    FILE *file() const { return m_file; }
private:
    FILE * const m_file;
};

void MaemoSftpConnection::transferFiles(const QList<SshDeploySpec> &deploySpecs)
{
    for (int i = 0; i < deploySpecs.count(); ++i) {
        const SshDeploySpec &deploySpec = deploySpecs.at(i);
        const QString &curSrcFile = deploySpec.srcFilePath();
        FileManager fileMgr(curSrcFile);
        if (!fileMgr.file())
            throw MaemoSshException(tr("Could not open file '%1'").arg(curSrcFile));
        const QString &curTgtFile = deploySpec.tgtFilePath();

        // TODO: Is the mkdir() method recursive? If not, we have to
        //       introduce a recursive version ourselves.
        if (deploySpec.mkdir()) {
            const QString &dir = QFileInfo(curTgtFile).path();
            sftp->mkdir(dir.toLatin1().data());
        }

        qDebug("Deploying file %s to %s.", qPrintable(curSrcFile), qPrintable(curTgtFile));

        if (!sftp->put(fileMgr.file(), curTgtFile.toLatin1().data())) {
            const QString &error = tr("Could not copy local file '%1' "
                    "to remote file '%2': %3").arg(curSrcFile, curTgtFile)
                    .arg(lastError());
            throw MaemoSshException(error);
        }
        emit fileCopied(curSrcFile);
    }
}

MaemoSftpConnection::Ptr MaemoSftpConnection::create(const MaemoDeviceConfig &devConf)
{
    return Ptr(new MaemoSftpConnection(devConf));
}

} // namespace Internal
} // namespace Qt4ProjectManager
