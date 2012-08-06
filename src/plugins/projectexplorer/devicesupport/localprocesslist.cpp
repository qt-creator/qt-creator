/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "localprocesslist.h"

#include <QProcess>
#include <QTimer>

#include <errno.h>
#include <string.h>

#ifdef Q_OS_UNIX
//#include <sys/types.h>
#include <signal.h>
#endif

namespace ProjectExplorer {
namespace Internal {
const int PsFieldWidth = 50;

LocalProcessList::LocalProcessList(const IDevice::ConstPtr &device, QObject *parent)
        : DeviceProcessList(device, parent),
          m_psProcess(new QProcess(this))
{
    connect(m_psProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(handlePsError()));
    connect(m_psProcess, SIGNAL(finished(int)), SLOT(handlePsFinished()));
}

void LocalProcessList::doUpdate()
{
    // We assume Desktop Unix systems to have a POSIX-compliant ps.
    // We need the padding because the command field can contain spaces, so we cannot split on those.
    m_psProcess->start(QString::fromLocal8Bit("ps -e -o pid=%1 -o comm=%1 -o args=%1")
                       .arg(QString(PsFieldWidth, QChar('x'))));
}

void LocalProcessList::handlePsFinished()
{
    QString errorString;
    if (m_psProcess->exitStatus() == QProcess::CrashExit) {
        errorString = tr("The ps process crashed.");
    } else if (m_psProcess->exitCode() != 0) {
        errorString = tr("The ps process failed with exit code %1.").arg(m_psProcess->exitCode());
    } else {
        const QString output = QString::fromLocal8Bit(m_psProcess->readAllStandardOutput());
        QStringList lines = output.split(QLatin1Char('\n'), QString::SkipEmptyParts);
        lines.removeFirst(); // Headers
        QList<DeviceProcess> processes;
        foreach (const QString &line, lines) {
            if (line.count() < 2*PsFieldWidth) {
                qDebug("%s: Ignoring malformed line", Q_FUNC_INFO);
                continue;
            }
            DeviceProcess p;
            p.pid = line.mid(0, PsFieldWidth).trimmed().toInt();
            p.exe = line.mid(PsFieldWidth, PsFieldWidth).trimmed();
            p.cmdLine = line.mid(2*PsFieldWidth).trimmed();
            processes << p;
        }
        reportProcessListUpdated(processes);
        return;
    }

    const QByteArray stderrData = m_psProcess->readAllStandardError();
    if (!stderrData.isEmpty()) {
        errorString += QLatin1Char('\n') + tr("The stderr output was: '%1'")
            .arg(QString::fromLocal8Bit(stderrData));
    }
    reportError(errorString);
}

void LocalProcessList::handlePsError()
{
    // Other errors are handled in the finished() handler.
    if (m_psProcess->error() == QProcess::FailedToStart)
        reportError(m_psProcess->errorString());
}

void LocalProcessList::doKillProcess(const DeviceProcess &process)
{
#ifdef Q_OS_UNIX
    if (kill(process.pid, SIGKILL) == -1)
        m_error = QString::fromLocal8Bit(strerror(errno));
    else
        m_error.clear();
    QTimer::singleShot(0, this, SLOT(reportDelayedKillStatus()));
#endif
}

void LocalProcessList::reportDelayedKillStatus()
{
    if (m_error.isEmpty())
        reportProcessKilled();
    else
        reportError(m_error);
}

} // namespace Internal
} // namespace RemoteLinux
