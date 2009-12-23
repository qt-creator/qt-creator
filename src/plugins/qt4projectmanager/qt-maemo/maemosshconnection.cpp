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

#include "/opt/ne7ssh/include/ne7ssh.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

namespace Qt4ProjectManager {
namespace Internal {
namespace {
    ne7ssh ssh;
}

MaemoSshConnection::Ptr MaemoSshConnection::connect(
    const MaemoDeviceConfig &devConf, bool shell)
{
    return MaemoSshConnection::Ptr(new MaemoSshConnection(devConf, shell));
}

MaemoSshConnection::MaemoSshConnection(const MaemoDeviceConfig &devConf,
                                       bool shell)
    : m_channel(-1), m_prompt(0), m_stopRequested(false)
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

    if (shell) {
        m_prompt = devConf.uname == QLatin1String("root") ? "# " : "$ ";
        if (!ssh.waitFor(m_channel, m_prompt, devConf.timeout)) {
            const QString error = tr("Could not start remote shell: %1").
                                  arg(ssh.errors()->pop(m_channel));
            ssh.close(m_channel);
            throw MaemoSshException(error);
        }
    }
}

MaemoSshConnection::~MaemoSshConnection()
{
    qDebug("%s", Q_FUNC_INFO);
    if (m_prompt) {
        ssh.send("exit\n", m_channel);
        ssh.waitFor(m_channel, m_prompt, 1);
    }

    ssh.close(m_channel);
}

void MaemoSshConnection::runCommand(const QString &command)
{
    if (!ssh.send((command + QLatin1String("\n")).toLatin1().data(),
                  m_channel)) {
        throw MaemoSshException(tr("Error running command: %1")
                                .arg(ssh.errors()->pop(m_channel)));
    }

    bool done;
    do {
        done = ssh.waitFor(m_channel, m_prompt, 3);
        const char * const error = ssh.errors()->pop(m_channel);
        if (error)
            throw MaemoSshException(tr("SSH error: %1").arg(error));
        const char * const output = ssh.read(m_channel);
        if (output)
            emit remoteOutput(QLatin1String(output));
    } while (!done && !m_stopRequested);
}

void MaemoSshConnection::stopCommand()
{
    m_stopRequested = true;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#endif
