/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "consoleprocess.h"

using namespace ProjectExplorer::Internal;

ConsoleProcess::ConsoleProcess(QObject *parent)
    : QObject(parent)
{
    m_isRunning = false;
    m_process = new QProcess(this);
}

ConsoleProcess::~ConsoleProcess()
{
}

static QString shellEscape(const QString &in)
{
    QString out = in;
    out.replace('\'', "'\''");
    out.prepend('\'');
    out.append('\'');
    return out;
}

bool ConsoleProcess::start(const QString &program, const QStringList &args)
{
    if (m_process->state() != QProcess::NotRunning)
        return false;
    QString shellArgs;
    shellArgs += QLatin1String("cd ");
    shellArgs += shellEscape(workingDirectory());
    shellArgs += QLatin1Char(';');
    shellArgs += shellEscape(program);
    foreach (const QString &arg, args) {
        shellArgs += QLatin1Char(' ');
        shellArgs += shellEscape(arg);
    }
    shellArgs += QLatin1String("; echo; echo \"Press enter to close this window\"; read DUMMY");

    m_process->setEnvironment(environment());

    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(processFinished(int, QProcess::ExitStatus)));

    m_process->start(QLatin1String("xterm"), QStringList() << QLatin1String("-e") << "/bin/sh" << "-c" << shellArgs);
    if (!m_process->waitForStarted())
        return false;
    emit processStarted();
    return true;
}

void ConsoleProcess::processFinished(int, QProcess::ExitStatus)
{
    emit processStopped();
}

bool ConsoleProcess::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

void ConsoleProcess::stop()
{
    m_process->terminate();
    m_process->waitForFinished();
}

qint64 ConsoleProcess::applicationPID() const
{
    return m_process->pid();
}

int ConsoleProcess::exitCode() const
{
    return m_process->exitCode();
}

