// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

static bool hasProcessOriginFromQtCreatorBuildDir(qint64 pid)
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

    qint64 parentPid = getppid();
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
