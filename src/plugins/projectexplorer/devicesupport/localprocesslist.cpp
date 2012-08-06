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

#include <QLibrary>
#include <QProcess>
#include <QTimer>

#include <errno.h>
#include <string.h>

#ifdef Q_OS_UNIX
//#include <sys/types.h>
#include <signal.h>
#endif

#ifdef Q_OS_WIN

// Enable Win API of XP SP1 and later
#define _WIN32_WINNT 0x0502
#include <windows.h>
#include <utils/winutils.h>
#include <tlhelp32.h>
#include <psapi.h>

#endif // Q_OS_WIN

namespace ProjectExplorer {
namespace Internal {
const int PsFieldWidth = 50;

#ifdef Q_OS_WIN

// Resolve QueryFullProcessImageNameW out of kernel32.dll due
// to incomplete MinGW import libs and it not being present
// on Windows XP.
static BOOL queryFullProcessImageName(HANDLE h, DWORD flags, LPWSTR buffer, DWORD *size)
{
    // Resolve required symbols from the kernel32.dll
    typedef BOOL (WINAPI *QueryFullProcessImageNameWProtoType)
                 (HANDLE, DWORD, LPWSTR, PDWORD);
    static QueryFullProcessImageNameWProtoType queryFullProcessImageNameW = 0;
    if (!queryFullProcessImageNameW) {
        QLibrary kernel32Lib(QLatin1String("kernel32.dll"), 0);
        if (kernel32Lib.isLoaded() || kernel32Lib.load())
            queryFullProcessImageNameW = (QueryFullProcessImageNameWProtoType)kernel32Lib.resolve("QueryFullProcessImageNameW");
    }
    if (!queryFullProcessImageNameW)
        return FALSE;
    // Read out process
    return (*queryFullProcessImageNameW)(h, flags, buffer, size);
}

static QString imageName(DWORD processId)
{
    QString  rc;
    HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION , FALSE, processId);
    if (handle == INVALID_HANDLE_VALUE)
        return rc;
    WCHAR buffer[MAX_PATH];
    DWORD bufSize = MAX_PATH;
    if (queryFullProcessImageName(handle, 0, buffer, &bufSize))
        rc = QString::fromUtf16(reinterpret_cast<const ushort*>(buffer));
    CloseHandle(handle);
    return rc;
}
#endif //Q_OS_WIN

LocalProcessList::LocalProcessList(const IDevice::ConstPtr &device, QObject *parent)
        : DeviceProcessList(device, parent),
          m_psProcess(new QProcess(this))
{
#ifdef Q_OS_UNIX
    connect(m_psProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(handlePsError()));
    connect(m_psProcess, SIGNAL(finished(int)), SLOT(handlePsFinished()));
#endif //Q_OS_UNIX
}

void LocalProcessList::doUpdate()
{
#ifdef Q_OS_UNIX
    // We assume Desktop Unix systems to have a POSIX-compliant ps.
    // We need the padding because the command field can contain spaces, so we cannot split on those.
    m_psProcess->start(QString::fromLocal8Bit("ps -e -o pid=%1 -o comm=%1 -o args=%1")
                       .arg(QString(PsFieldWidth, QChar('x'))));
#endif
#ifdef Q_OS_WIN
    QTimer::singleShot(0, this, SLOT(handleWindowsUpdate()));
#endif
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

void LocalProcessList::handleWindowsUpdate()
{
    QList<DeviceProcess> processes;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return;

    for (bool hasNext = Process32First(snapshot, &pe); hasNext; hasNext = Process32Next(snapshot, &pe)) {
        DeviceProcess p;
        p.pid = pe.th32ProcessID;
        p.exe = QString::fromUtf16(reinterpret_cast<ushort*>(pe.szExeFile));
        p.cmdLine = imageName(pe.th32ProcessID);
        if (p.cmdLine.isEmpty())
            p.cmdLine = p.exe;
        processes << p;
    }
    CloseHandle(snapshot);

    reportProcessListUpdated(processes);
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
