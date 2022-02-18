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

#include "sshremoteprocess.h"

#include "sshlogging_p.h"
#include "sshsettings.h"

#include <utils/commandline.h>
#include <utils/qtcassert.h>

#include <QTimer>

/*!
    \class QSsh::SshRemoteProcess

    \brief The SshRemoteProcess class implements an SSH channel for running a
    remote process.

    Objects are created via SshConnection::createRemoteProcess.
    The process is started via the start() member function.
    If the process needs a pseudo terminal, you can request one
    via requestTerminal() before calling start().
 */

using namespace QSsh::Internal;

namespace QSsh {

SshRemoteProcess::SshRemoteProcess(const QString &command, const QStringList &connectionArgs)
    : SshProcess()
{
    m_remoteCommand = command;
    m_connectionArgs = connectionArgs;

    connect(this, &QtcProcess::finished, this, [this] {
        QString error;
        if (exitStatus() == QProcess::CrashExit)
            error = tr("The ssh process crashed: %1").arg(errorString());
        emit done(error);
    });
    connect(this, &QtcProcess::errorOccurred, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            emit done(errorString());
    });
}

void SshRemoteProcess::start()
{
    QTC_ASSERT(!isRunning(), return);
    const Utils::CommandLine cmd = fullLocalCommandLine();
    if (!m_displayName.isEmpty()) {
        Utils::Environment env = environment();
        env.set("DISPLAY", m_displayName);
        setEnvironment(env);
    }
    qCDebug(sshLog) << "starting remote process:" << cmd.toUserOutput();
    setCommand(cmd);
    QtcProcess::start();
}

void SshRemoteProcess::requestX11Forwarding(const QString &displayName)
{
    m_displayName = displayName;
}

Utils::CommandLine SshRemoteProcess::fullLocalCommandLine(bool inTerminal) const
{
    Utils::CommandLine cmd{SshSettings::sshFilePath()};

    if (!m_displayName.isEmpty())
        cmd.addArg("-X");
    if (inTerminal)
        cmd.addArg("-tt");

    cmd.addArg("-q");
    cmd.addArgs(m_connectionArgs);

    if (!m_remoteCommand.isEmpty())
        cmd.addArg(m_remoteCommand);

    return cmd;
}

} // namespace QSsh
