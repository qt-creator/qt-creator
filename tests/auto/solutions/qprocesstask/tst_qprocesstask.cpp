// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tasking/qprocesstask.h>

#include <QtTest>

using namespace Tasking;

class tst_QProcessTask : public QObject
{
    Q_OBJECT

private slots:
    void qProcessTask();
};

#ifdef Q_OS_WIN
static const char s_processName[] = "testsleep.exe";
#else
static const char s_processName[] = "testsleep";
#endif

void tst_QProcessTask::qProcessTask()
{
    const auto setupProcess = [](QProcess &process) {
        process.setProgram(QLatin1String(PROCESS_TESTAPP) + '/' + s_processName);
    };

    {
        TaskTree taskTree({QProcessTask(setupProcess)});
        taskTree.start();
        QTRY_VERIFY(taskTree.isRunning());
    }
    QProcessDeleter::deleteAll();
}

QTEST_GUILESS_MAIN(tst_QProcessTask)

#include "tst_qprocesstask.moc"
