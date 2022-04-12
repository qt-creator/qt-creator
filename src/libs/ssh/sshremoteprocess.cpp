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
#include <utils/processinterface.h>
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
using namespace Utils;

namespace QSsh {

SshRemoteProcess::SshRemoteProcess(const QString &command, const QStringList &connectionArgs)
    : QtcProcess()
{
    setupSshEnvironment(this);
    m_remoteCommand = command;
    m_connectionArgs = connectionArgs;
}

void SshRemoteProcess::emitFinished()
{
    if (exitStatus() == QProcess::CrashExit)
        m_errorString = tr("The ssh process crashed: %1").arg(errorString());
    QtcProcess::emitFinished();
}

void SshRemoteProcess::startImpl()
{
    QTC_ASSERT(!isRunning(), return);
    m_errorString.clear();
    const CommandLine cmd = fullLocalCommandLine();
    if (!m_displayName.isEmpty()) {
        Environment env = environment();
        env.set("DISPLAY", m_displayName);
        setEnvironment(env);
    }
    qCDebug(sshLog) << "starting remote process:" << cmd.toUserOutput();
    setCommand(cmd);
    QtcProcess::startImpl();
}

ProcessResultData SshRemoteProcess::resultData() const
{
    ProcessResultData result = QtcProcess::resultData();
    if (!m_errorString.isEmpty())
        result.m_errorString = m_errorString;
    return result;
}

void SshRemoteProcess::requestX11Forwarding(const QString &displayName)
{
    m_displayName = displayName;
}

CommandLine SshRemoteProcess::fullLocalCommandLine(bool inTerminal) const
{
    CommandLine cmd {SshSettings::sshFilePath()};

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

bool SshRemoteProcess::setupSshEnvironment(QtcProcess *process)
{
    Environment env = process->hasEnvironment() ? process->environment()
                                                : Environment::systemEnvironment();
    const bool hasDisplay = env.hasKey("DISPLAY") && (env.value("DISPLAY") != QString(":0"));
    if (SshSettings::askpassFilePath().exists()) {
        env.set("SSH_ASKPASS", SshSettings::askpassFilePath().toUserOutput());

        // OpenSSH only uses the askpass program if DISPLAY is set, regardless of the platform.
        if (!env.hasKey("DISPLAY"))
            env.set("DISPLAY", ":0");
    }
    process->setEnvironment(env);

    // Otherwise, ssh will ignore SSH_ASKPASS and read from /dev/tty directly.
    process->setDisableUnixTerminal();
    return hasDisplay;
}

} // namespace QSsh
