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
                 qPrintable(args.join(QLatin1String(" "))));
    QProcess p;
    p.start(cmd, args);
    if (!p.waitForStarted())
        return -2;
    p.closeWriteChannel();

    QByteArray stdOut;
    QByteArray stdErr;
    if (!Utils::SynchronousProcess::readDataFromProcess(p, 2000, &stdOut, &stdErr)) {
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
