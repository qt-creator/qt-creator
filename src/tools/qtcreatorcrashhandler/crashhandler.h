// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

class ApplicationInfo;
class CrashHandlerPrivate;

class CrashHandler : public QObject
{
    Q_OBJECT
public:
    enum RestartCapability { EnableRestart, DisableRestart };

    explicit CrashHandler(pid_t pid,
                          const QString &signalName,
                          const QString &appName,
                          RestartCapability restartCap = EnableRestart,
                          QObject *parent = 0);
    ~CrashHandler();

    void run();

    void onError(const QString &errorMessage);
    void onBacktraceChunk(const QString &chunk);
    void onBacktraceFinished(const QString &backtrace);

    void openBugTracker();
    void restartApplication();
    void debugApplication();

private:
    bool collectRestartAppData();

    enum WaitMode { WaitForExit, DontWaitForExit };
    static void runCommand(QStringList commandLine, QStringList environment, WaitMode waitMode);

    CrashHandlerPrivate *d;
};
