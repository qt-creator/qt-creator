/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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

bool ConsoleProcess::start(const QString &program, const QStringList &args)
{
    if (m_process->state() != QProcess::NotRunning)
        return false;
    QString shellArgs;
    shellArgs += QLatin1String("cd ");
    shellArgs += workingDirectory();
    shellArgs += QLatin1Char(';');
    shellArgs += program;
    foreach (const QString &arg, args) {
        shellArgs += QLatin1Char(' ');
        shellArgs += QLatin1Char('\'');
        shellArgs += arg;
        shellArgs += QLatin1Char('\'');
    }
    shellArgs += QLatin1String("; echo; echo \"Press enter to close this window\"; read");

    m_process->setEnvironment(environment());

    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(processFinished(int, QProcess::ExitStatus)));

    m_process->start(QLatin1String("xterm"), QStringList() << QLatin1String("-e") << shellArgs);
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

