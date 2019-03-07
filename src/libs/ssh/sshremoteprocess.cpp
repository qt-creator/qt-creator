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

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QTimer>

#include <cstring>
#include <cstdlib>

/*!
    \class QSsh::SshRemoteProcess

    \brief The SshRemoteProcess class implements an SSH channel for running a
    remote process.

    Objects are created via SshConnection::createRemoteProcess.
    The process is started via the start() member function.
    If the process needs a pseudo terminal, you can request one
    via requestTerminal() before calling start().
 */

namespace QSsh {
using namespace Internal;

struct SshRemoteProcess::SshRemoteProcessPrivate
{
    QByteArray remoteCommand;
    QStringList connectionArgs;
    QString displayName;
    bool useTerminal = false;
};

SshRemoteProcess::SshRemoteProcess(const QByteArray &command, const QStringList &connectionArgs)
    : d(new SshRemoteProcessPrivate)
{
    d->remoteCommand = command;
    d->connectionArgs = connectionArgs;

    connect(this,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [this] {
        QString error;
        if (exitStatus() == QProcess::CrashExit)
            error = tr("The ssh process crashed: %1").arg(errorString());
        else if (exitCode() == 255)
            error = tr("Remote process crashed.");
        emit done(error);
    });
    connect(this, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            emit done(errorString());
    });
}

void SshRemoteProcess::doStart()
{
    QTC_ASSERT(!isRunning(), return);
    const QStringList args = fullLocalCommandLine();
    if (!d->displayName.isEmpty()) {
        QProcessEnvironment env = processEnvironment();
        env.insert("DISPLAY", d->displayName);
        setProcessEnvironment(env);
    }
    qCDebug(sshLog) << "starting remote process:" << QDir::toNativeSeparators(args.first())
                    << args;
    QProcess::start(args.first(), args.mid(1));
}

SshRemoteProcess::~SshRemoteProcess()
{
    delete d;
}

void SshRemoteProcess::requestTerminal()
{
    d->useTerminal = true;
}

void SshRemoteProcess::requestX11Forwarding(const QString &displayName)
{
    d->displayName = displayName;
}

void SshRemoteProcess::start()
{
    QTimer::singleShot(0, this, &SshRemoteProcess::doStart);
}

bool SshRemoteProcess::isRunning() const
{
    return state() == QProcess::Running;
}

QStringList SshRemoteProcess::fullLocalCommandLine() const
{
    QStringList args = QStringList("-q") << d->connectionArgs;
    if (d->useTerminal)
        args.prepend("-tt");
    if (!d->displayName.isEmpty())
        args.prepend("-X");
    if (!d->remoteCommand.isEmpty())
        args << QLatin1String(d->remoteCommand);
    args.prepend(SshSettings::sshFilePath().toString());
    return args;
}

} // namespace QSsh
