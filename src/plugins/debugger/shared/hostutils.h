/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
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

#ifndef DEBUGGER_HOSTUTILS_H
#define DEBUGGER_HOSTUTILS_H

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

struct ProcData
{
    QString ppid;
    QString name;
    QString image;
    QString state;
};


QList<ProcData> hostProcessList();

#ifdef Q_OS_WIN

// Resume a suspended thread by id.
bool winResumeThread(unsigned long dwThreadId, QString *errorMessage);

unsigned long winGetCurrentProcessId();

bool isWinProcessBeingDebugged(unsigned long pid);

// Special exception codes.
enum
{
    winExceptionCppException = 0xe06d7363,
    winExceptionStartupCompleteTrap = 0x406d1388,
    winExceptionRpcServerUnavailable = 0x6ba,
    winExceptionRpcServerInvalid = 0x6a6,
    winExceptionDllNotFound = 0xc0000135,
    winExceptionDllEntryPointNoFound = 0xc0000139,
    winExceptionDllInitFailed = 0xc0000142,
    winExceptionMissingSystemFile = 0xc0000143,
    winExceptionAppInitFailed = 0xc0000143,
    winExceptionWX86Breakpoint = 0x4000001f,
    winExceptionCtrlPressed = 0x40010005
};

// Format windows Exception
void formatWindowsException(unsigned long code, quint64 address,
                            unsigned long flags, quint64 info1, quint64 info2,
                            QTextStream &str);
// Check for access violation, etc.
bool isFatalWinException(long code);

// Check for EXCEPTION_BREAKPOINT, EXCEPTION_SINGLE_STEP
bool isDebuggerWinException(long code);

#endif // defined(Q_OS_WIN)

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_HOSTUTILS_H
