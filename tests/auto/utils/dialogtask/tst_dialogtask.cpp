// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/dialogtask.h>

#include <QtTaskTree/qtasktree.h>

#include <QDialog>
#include <QTest>
#include <QTimer>

using namespace QtTaskTree;
using namespace Utils;

using namespace std::chrono;
using namespace std::chrono_literals;

class tst_DialogTask : public QObject
{
    Q_OBJECT

private slots:
    void accepted();
    void rejected();
    void customResultCode();
    void forcesDeleteOnCloseOff();
    void cancel();

private:
    struct Outcome { DoneWith doneWith; int result; };

    // Runs a single DialogTask<QDialog>. The action is invoked from the setup
    // handler with the owned dialog, typically to schedule its termination.
    template <typename Action>
    static Outcome runDialog(Action action)
    {
        int captured = -1;
        const auto onSetup = [action](DialogWrapper<QDialog> &task) {
            action(task.dialog());
        };
        const auto onDone = [&captured](const DialogWrapper<QDialog> &task, DoneWith) {
            captured = task.result();
        };
        QTaskTree taskTree({DialogTask<QDialog>(onSetup, onDone).withTimeout(5000ms)});
        const DoneWith doneWith = taskTree.runBlocking();
        return {doneWith, captured};
    }
};

void tst_DialogTask::accepted()
{
    const Outcome outcome = runDialog([](QDialog *dialog) {
        QTimer::singleShot(0, dialog, &QDialog::accept);
    });
    QCOMPARE(outcome.doneWith, DoneWith::Success);
    QCOMPARE(outcome.result, int(QDialog::Accepted));
}

void tst_DialogTask::rejected()
{
    const Outcome outcome = runDialog([](QDialog *dialog) {
        QTimer::singleShot(0, dialog, &QDialog::reject);
    });
    QCOMPARE(outcome.doneWith, DoneWith::Error);
    QCOMPARE(outcome.result, int(QDialog::Rejected));
}

void tst_DialogTask::customResultCode()
{
    // A result code other than Accepted maps to Error, but is still reported by result().
    const Outcome outcome = runDialog([](QDialog *dialog) {
        QTimer::singleShot(0, dialog, [dialog] { dialog->done(42); });
    });
    QCOMPARE(outcome.doneWith, DoneWith::Error);
    QCOMPARE(outcome.result, 42);
}

void tst_DialogTask::forcesDeleteOnCloseOff()
{
    // Setting WA_DeleteOnClose must be overridden by the task (the dialog is a
    // value member); it warns and proceeds without crashing.
    QTest::ignoreMessage(QtWarningMsg,
        "DialogTask: Qt::WA_DeleteOnClose is managed by the task; forcing it off.");
    const Outcome outcome = runDialog([](QDialog *dialog) {
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        QTimer::singleShot(0, dialog, &QDialog::accept);
    });
    QCOMPARE(outcome.doneWith, DoneWith::Success);
    QCOMPARE(outcome.result, int(QDialog::Accepted));
}

void tst_DialogTask::cancel()
{
    // The dialog never finishes; a parallel timeout errors first and cancels it.
    // The still-open dialog must be torn down without crashing.
    const Group recipe {
        parallel,
        stopOnError,
        DialogTask<QDialog>([](DialogWrapper<QDialog> &) {}),
        timeoutTask(50ms, DoneResult::Error)
    };
    QTaskTree taskTree({recipe});
    const DoneWith doneWith = taskTree.runBlocking();
    QCOMPARE(doneWith, DoneWith::Error);
    QCOMPARE(taskTree.isRunning(), false);
}

QTEST_MAIN(tst_DialogTask)

#include "tst_dialogtask.moc"
