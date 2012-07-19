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

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>

#if defined(Q_OS_LINUX)
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#endif

namespace Debugger {
namespace Internal {

void dumpBacktrace(int maxdepth)
{
    if (maxdepth == -1)
        maxdepth = 200;
#if defined(Q_OS_LINUX)
    void *bt[200] = {0};
    qDebug() << "BACKTRACE:";
    int size = backtrace(bt, sizeof(bt) / sizeof(bt[0]));
    for (int i = 0; i < qMin(size, maxdepth); i++)
        qDebug() << "0x" + QByteArray::number(quintptr(bt[i]), 16);
    QProcess proc;
    QStringList args;
    args.append(QLatin1String("-e"));
    args.append(QCoreApplication::arguments().at(0));
    proc.start(QLatin1String("addr2line"), args);
    proc.waitForStarted();
    for (int i = 0; i < qMin(size, maxdepth); i++)
        proc.write("0x" + QByteArray::number(quintptr(bt[i]), 16) + '\n');
    proc.closeWriteChannel();
    QByteArray out = proc.readAllStandardOutput();
    qDebug() << QCoreApplication::arguments().at(0);
    qDebug() << out;
    proc.waitForFinished();
    out = proc.readAllStandardOutput();
    qDebug() << out;
#endif
}

/*
void installSignalHandlers()
{
#if defined(Q_OS_LINUX)
    struct sigaction SignalAction;

    SignalAction.sa_sigaction = handler;
    sigemptyset(&SignalAction.sa_mask);
    SignalAction.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &SignalAction, NULL);
    sigaction(SIGABRT, &SignalAction, NULL);
#endif
}
*/

} // namespace Internal
} // namespace Debugger
