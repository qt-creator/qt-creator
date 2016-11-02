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

#include "crashhandler.h"
#include "utils.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QStyle>
#include <QTextStream>

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static void printErrorAndExit()
{
    QTextStream err(stderr);
    err << QString::fromLatin1("This crash handler will be called by Qt Creator itself. "
                               "Do not call this manually.\n");
    exit(EXIT_FAILURE);
}

static bool hasProcessOriginFromQtCreatorBuildDir(Q_PID pid)
{
    const QString executable = QFile::symLinkTarget(QString::fromLatin1("/proc/%1/exe")
        .arg(QString::number(pid)));
    return executable.contains("qtcreator");
}

// Called by signal handler of crashing application.
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QLatin1String(APPLICATION_NAME));
    app.setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical));

    // Parse arguments.
    QCommandLineParser parser;
    parser.addPositionalArgument("signal-name", QString());
    parser.addPositionalArgument("app-name", QString());
    const QCommandLineOption disableRestartOption("disable-restart");
    parser.addOption(disableRestartOption);
    parser.process(app);

    // Check usage.
    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.size() != 2)
        printErrorAndExit();

    Q_PID parentPid = getppid();
    if (!hasProcessOriginFromQtCreatorBuildDir(parentPid))
        printErrorAndExit();

    // Run.
    const QString signalName = parser.positionalArguments().at(0);
    const QString appName = parser.positionalArguments().at(1);
    CrashHandler::RestartCapability restartCap = CrashHandler::EnableRestart;
    if (parser.isSet(disableRestartOption))
        restartCap = CrashHandler::DisableRestart;
    CrashHandler *crashHandler = new CrashHandler(parentPid, signalName, appName, restartCap);
    crashHandler->run();

    return app.exec();
}
