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

#include "consoleprocess_p.h"

namespace Utils {

ConsoleProcess::~ConsoleProcess()
{
    stop();
    delete d;
}

void ConsoleProcess::setMode(Mode m)
{
    d->m_mode = m;
}

ConsoleProcess::Mode ConsoleProcess::mode() const
{
    return d->m_mode;
}

QString ConsoleProcess::modeOption(Mode m)
{
    switch (m) {
        case Debug:
        return QLatin1String("debug");
        case Suspend:
        return QLatin1String("suspend");
        case Run:
        break;
    }
    return QLatin1String("run");
}

qint64 ConsoleProcess::applicationPID() const
{
    return d->m_appPid;
}

int ConsoleProcess::exitCode() const
{
    return d->m_appCode;
} // This will be the signal number if exitStatus == CrashExit

QProcess::ExitStatus ConsoleProcess::exitStatus() const
{
    return d->m_appStatus;
}

void ConsoleProcess::setWorkingDirectory(const QString &dir)
{
    d->m_workingDir = dir;
}

QString ConsoleProcess::workingDirectory() const
{
    return d->m_workingDir;
}

void ConsoleProcess::setEnvironment(const Environment &env)
{
    d->m_environment = env;
}

Environment ConsoleProcess::environment() const
{
    return d->m_environment;
}

QProcess::ProcessError ConsoleProcess::error() const
{
    return d->m_error;
}

QString ConsoleProcess::errorString() const
{
    return d->m_errorString;
}

QString ConsoleProcess::msgCommChannelFailed(const QString &error)
{
    return tr("Cannot set up communication channel: %1").arg(error);
}

QString ConsoleProcess::msgPromptToClose()
{
    //! Showed in a terminal which might have
    //! a different character set on Windows.
    return tr("Press <RETURN> to close this window...");
}

QString ConsoleProcess::msgCannotCreateTempFile(const QString &why)
{
    return tr("Cannot create temporary file: %1").arg(why);
}

QString ConsoleProcess::msgCannotWriteTempFile()
{
    return tr("Cannot write temporary file. Disk full?");
}

QString ConsoleProcess::msgCannotCreateTempDir(const QString & dir, const QString &why)
{
    return tr("Cannot create temporary directory \"%1\": %2").arg(dir, why);
}

QString ConsoleProcess::msgUnexpectedOutput(const QByteArray &what)
{
    return tr("Unexpected output from helper program (%1).").arg(QString::fromLatin1(what));
}

QString ConsoleProcess::msgCannotChangeToWorkDir(const QString & dir, const QString &why)
{
    return tr("Cannot change to working directory \"%1\": %2").arg(dir, why);
}

QString ConsoleProcess::msgCannotExecute(const QString & p, const QString &why)
{
    return tr("Cannot execute \"%1\": %2").arg(p, why);
}

void ConsoleProcess::emitError(QProcess::ProcessError err, const QString &errorString)
{
    d->m_error = err;
    d->m_errorString = errorString;
    emit error(err);
    emit processError(errorString);
}

}
