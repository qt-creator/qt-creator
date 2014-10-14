/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gdbprocess.h"

#include <debugger/debuggerconstants.h>
#include <debugger/procinterrupt.h>

namespace Debugger {
namespace Internal {

GdbProcess::GdbProcess(QObject *parent) : QObject(parent)
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

QByteArray GdbProcess::readAllStandardOutput()
{
    return m_gdbProc.readAllStandardOutput();
}

QByteArray GdbProcess::readAllStandardError()
{
    return m_gdbProc.readAllStandardError();
}

void GdbProcess::start(const QString &cmd, const QStringList &args)
{
    m_gdbProc.setCommand(cmd, Utils::QtcProcess::joinArgs(args));
    m_gdbProc.start();
}

bool GdbProcess::waitForStarted()
{
    return m_gdbProc.waitForStarted();
}

qint64 GdbProcess::write(const QByteArray &data)
{
    return m_gdbProc.write(data);
}

void GdbProcess::kill()
{
    m_gdbProc.kill();
}

bool GdbProcess::interrupt()
{
    long pid = Utils::qPidToPid(m_gdbProc.pid());
    return interruptProcess(pid, GdbEngineType, &m_errorString);
}

void GdbProcess::setUseCtrlCStub(bool enable)
{
    m_gdbProc.setUseCtrlCStub(enable);
}

void GdbProcess::winInterruptByCtrlC()
{
    m_gdbProc.interrupt();
}

QProcess::ProcessState GdbProcess::state() const
{
    return m_gdbProc.state();
}

QString GdbProcess::errorString() const
{
    return m_errorString + m_gdbProc.errorString();
}

QProcessEnvironment GdbProcess::processEnvironment() const
{
    return m_gdbProc.processEnvironment();
}

void GdbProcess::setProcessEnvironment(const QProcessEnvironment &env)
{
    m_gdbProc.setProcessEnvironment(env);
}

void GdbProcess::setEnvironment(const QStringList &env)
{
    m_gdbProc.setEnvironment(Utils::Environment(env));
}

void GdbProcess::setWorkingDirectory(const QString &dir)
{
    m_gdbProc.setWorkingDirectory(dir);
}

} // namespace Internal
} // namespace Debugger

