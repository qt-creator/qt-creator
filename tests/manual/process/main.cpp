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

#include "mainwindow.h"

#include <utils/synchronousprocess.h>
#include <QApplication>
#include <QDebug>
#include <QTimer>

#include <cstdio>

static const char usage[] =
"Tests timeout behaviour of Utils:SynchronousProcess.\n"
"Usage:\n"
"   1) Test Utils:SynchronousProcess (graphically)\n"
"   process <cmd> <args>\n"
"   2) Test synchronous helpers of Utils:SynchronousProcess (tty)\n"
"   process -s <cmd> <args>\n\n"
"slowprocess.sh is provided as an example script that produces slow\n"
"output. It takes an option -e to switch to stderr\n"
"No timeout should occur.\n";

static int testSynchronous(const QString &cmd, const QStringList &args)
{
    std::fprintf(stdout, "testSynchronous %s %s\n", qPrintable(cmd),
                 qPrintable(args.join(QLatin1Char(' '))));
    QProcess p;
    p.start(cmd, args);
    if (!p.waitForStarted())
        return -2;
    p.closeWriteChannel();

    QByteArray stdOut;
    QByteArray stdErr;
    if (!Utils::SynchronousProcess::readDataFromProcess(p, 2, &stdOut, &stdErr)) {
        std::fputs("Timeout", stderr);
        return -3;
    }
    std::fputs(stdOut, stdout);
    std::fputs(stdErr, stderr);
    return p.exitCode();
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::fputs(usage, stdout);
        return -1;
    }
    const bool synchronous = argc > 1 && !qstrcmp(argv[1], "-s");
    int ex = 0;
    if (synchronous) {
        const QString cmd = QString::fromLocal8Bit(argv[2]);
        QStringList args;
        for (int i = 3; i < argc; i++)
            args += QString::fromLocal8Bit(argv[i]);
        ex = testSynchronous(cmd, args);
    } else {
        QApplication app(argc, argv);
        MainWindow mw;
        mw.show();
        ex = app.exec();
    }
    return ex;
}
