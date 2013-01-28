/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "localgdbprocess.h"

#include "procinterrupt.h"
#include "debuggerconstants.h"

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

#include <utils/qtcassert.h>


namespace Debugger {
namespace Internal {

LocalGdbProcess::LocalGdbProcess(QObject *parent) : AbstractGdbProcess(parent)
{
    connect(&m_gdbProc, SIGNAL(error(QProcess::ProcessError)),
            this, SIGNAL(error(QProcess::ProcessError)));
    connect(&m_gdbProc, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SIGNAL(finished(int,QProcess::ExitStatus)));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()),
            this, SIGNAL(readyReadStandardOutput()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()),
            this, SIGNAL(readyReadStandardError()));
}

QByteArray LocalGdbProcess::readAllStandardOutput()
{
    return m_gdbProc.readAllStandardOutput();
}

QByteArray LocalGdbProcess::readAllStandardError()
{
    return m_gdbProc.readAllStandardError();
}

void LocalGdbProcess::start(const QString &cmd, const QStringList &args)
{
    m_gdbProc.start(cmd, args);
}

bool LocalGdbProcess::waitForStarted()
{
    return m_gdbProc.waitForStarted();
}

qint64 LocalGdbProcess::write(const QByteArray &data)
{
    return m_gdbProc.write(data);
}

void LocalGdbProcess::kill()
{
    m_gdbProc.kill();
}

bool LocalGdbProcess::interrupt()
{
    long pid;
#ifdef Q_OS_WIN
    pid = Utils::winQPidToPid(m_gdbProc.pid());
#else
    pid = m_gdbProc.pid();
#endif
    return interruptProcess(pid, GdbEngineType, &m_errorString);
}

QProcess::ProcessState LocalGdbProcess::state() const
{
    return m_gdbProc.state();
}

QString LocalGdbProcess::errorString() const
{
    return m_errorString + m_gdbProc.errorString();
}

QProcessEnvironment LocalGdbProcess::processEnvironment() const
{
    return m_gdbProc.processEnvironment();
}

void LocalGdbProcess::setProcessEnvironment(const QProcessEnvironment &env)
{
    m_gdbProc.setProcessEnvironment(env);
}

void LocalGdbProcess::setEnvironment(const QStringList &env)
{
    m_gdbProc.setEnvironment(env);
}

void LocalGdbProcess::setWorkingDirectory(const QString &dir)
{
    m_gdbProc.setWorkingDirectory(dir);
}

} // namespace Internal
} // namespace Debugger
