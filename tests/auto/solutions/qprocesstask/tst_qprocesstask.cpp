// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTaskTree/qprocesstask.h>

#include <QTest>

using namespace QtTaskTree;

class tst_QProcessTask : public QObject
{
    Q_OBJECT

private slots:
    void qProcessTask();
};

void tst_QProcessTask::qProcessTask()
{
    const auto setupProcess = [](QProcess &process) {
        process.setProgram(QLatin1String(PROCESS_TESTAPP));
    };

    {
        QTaskTree taskTree({QProcessTask(setupProcess)});
        taskTree.start();
        QTRY_VERIFY(taskTree.isRunning());
    }
    QProcessDeleter::syncAll();
}

QTEST_GUILESS_MAIN(tst_QProcessTask)

#include "tst_qprocesstask.moc"
