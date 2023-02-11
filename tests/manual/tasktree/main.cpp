// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskwidget.h"

#include <utils/asynctask.h>
#include <utils/futuresynchronizer.h>
#include <utils/layoutbuilder.h>
#include <utils/theme/theme_p.h>

#include <QApplication>
#include <QCheckBox>
#include <QProgressBar>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>

using namespace Utils;

// TODO: make tasks cancellable
static void sleepInThread(QPromise<void> &promise, int seconds, bool reportSuccess)
{
    QThread::sleep(seconds);
    if (!reportSuccess)
        promise.future().cancel();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    setCreatorTheme(new Theme("default", &app));

    QWidget mainWidget;
    mainWidget.setWindowTitle("Task Tree Demo");

    // Non-task GUI

    QToolButton *startButton = new QToolButton();
    startButton->setText("Start");
    QToolButton *stopButton = new QToolButton();
    stopButton->setText("Stop");
    QToolButton *resetButton = new QToolButton();
    resetButton->setText("Reset");
    QProgressBar *progressBar = new QProgressBar();
    QCheckBox *synchronizerCheckBox = new QCheckBox("Use Future Synchronizer");
    synchronizerCheckBox->setChecked(true);
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    QWidget *scrollAreaWidget = new QWidget();

    // Task GUI

    QList<StateWidget *> allStateWidgets;

    auto createGroupWidget = [&allStateWidgets] {
        auto *widget = new GroupWidget();
        allStateWidgets.append(widget);
        return widget;
    };
    auto createTaskWidget = [&allStateWidgets] {
        auto *widget = new TaskWidget();
        allStateWidgets.append(widget);
        return widget;
    };

    GroupWidget *rootGroup = createGroupWidget();

    GroupWidget *groupTask_1 = createGroupWidget();
    TaskWidget *task_1_1 = createTaskWidget();
    TaskWidget *task_1_2 = createTaskWidget();
    TaskWidget *task_1_3 = createTaskWidget();

    TaskWidget *task_2 = createTaskWidget();
    TaskWidget *task_3 = createTaskWidget();

    GroupWidget *groupTask_4 = createGroupWidget();
    TaskWidget *task_4_1 = createTaskWidget();
    TaskWidget *task_4_2 = createTaskWidget();
    GroupWidget *groupTask_4_3 = createGroupWidget();
    TaskWidget *task_4_3_1 = createTaskWidget();
    TaskWidget *task_4_3_2 = createTaskWidget();
    TaskWidget *task_4_3_3 = createTaskWidget();
    TaskWidget *task_4_3_4 = createTaskWidget();
    TaskWidget *task_4_4 = createTaskWidget();
    TaskWidget *task_4_5 = createTaskWidget();

    TaskWidget *task_5 = createTaskWidget();

    // Task initial configuration

    task_1_2->setBusyTime(2);
    task_1_2->setSuccess(false);
    task_1_3->setBusyTime(3);
    task_4_3_1->setBusyTime(4);
    task_4_3_2->setBusyTime(2);
    task_4_3_3->setBusyTime(1);
    task_4_3_4->setBusyTime(3);
    task_4_3_4->setSuccess(false);
    task_4_4->setBusyTime(6);
    task_4_4->setBusyTime(3);

    groupTask_1->setWorkflowPolicy(Tasking::WorkflowPolicy::ContinueOnDone);
    groupTask_4->setWorkflowPolicy(Tasking::WorkflowPolicy::Optional);
    groupTask_4_3->setExecuteMode(ExecuteMode::Parallel);
    groupTask_4_3->setWorkflowPolicy(Tasking::WorkflowPolicy::StopOnError);

    // Task layout

    {
        using namespace Layouting;

        Column {
            TaskGroup { rootGroup, {
                TaskGroup { groupTask_1, {
                    task_1_1, hr,
                    task_1_2, hr,
                    task_1_3,
                }}, hr,
                task_2, hr,
                task_3, hr,
                TaskGroup { groupTask_4, {
                    task_4_1, hr,
                    task_4_2, hr,
                    TaskGroup { groupTask_4_3, {
                        task_4_3_1, hr,
                        task_4_3_2, hr,
                        task_4_3_3, hr,
                        task_4_3_4,
                    }}, hr,
                    task_4_4, hr,
                    task_4_5,
                }}, hr,
                task_5
            }}, st
        }.attachTo(scrollAreaWidget);

        scrollArea->setWidget(scrollAreaWidget);

        Column {
            Row { startButton, stopButton, resetButton, synchronizerCheckBox, progressBar },
            hr,
            scrollArea
        }.attachTo(&mainWidget);
    }

    // Task tree creator (takes configuation from GUI)

    std::unique_ptr<TaskTree> taskTree;
    FutureSynchronizer synchronizer;
    synchronizer.setCancelOnWait(true);

    auto treeRoot = [&] {
        using namespace Tasking;

        auto taskItem = [sync = &synchronizer, synchronizerCheckBox](TaskWidget *widget) {
            const auto setupHandler = [=](AsyncTask<void> &task) {
                task.setConcurrentCallData(sleepInThread, widget->busyTime(), widget->isSuccess());
                if (synchronizerCheckBox->isChecked())
                    task.setFutureSynchronizer(sync);
                widget->setState(State::Running);
            };
            const auto doneHandler = [widget](const AsyncTask<void> &) {
                widget->setState(State::Done);
            };
            const auto errorHandler = [widget](const AsyncTask<void> &) {
                widget->setState(State::Error);
            };
            return Async<void>(setupHandler, doneHandler, errorHandler);
        };

        const Group root {
            rootGroup->executeMode(),
            Workflow(rootGroup->workflowPolicy()),
            OnGroupSetup([rootGroup] { rootGroup->setState(State::Running); }),
            OnGroupDone([rootGroup] { rootGroup->setState(State::Done); }),
            OnGroupError([rootGroup] { rootGroup->setState(State::Error); }),

            Group {
                groupTask_1->executeMode(),
                Workflow(groupTask_1->workflowPolicy()),
                OnGroupSetup([groupTask_1] { groupTask_1->setState(State::Running); }),
                OnGroupDone([groupTask_1] { groupTask_1->setState(State::Done); }),
                OnGroupError([groupTask_1] { groupTask_1->setState(State::Error); }),

                taskItem(task_1_1),
                taskItem(task_1_2),
                taskItem(task_1_3)
            },
            taskItem(task_2),
            taskItem(task_3),
            Group {
                groupTask_4->executeMode(),
                Workflow(groupTask_4->workflowPolicy()),
                OnGroupSetup([groupTask_4] { groupTask_4->setState(State::Running); }),
                OnGroupDone([groupTask_4] { groupTask_4->setState(State::Done); }),
                OnGroupError([groupTask_4] { groupTask_4->setState(State::Error); }),

                taskItem(task_4_1),
                taskItem(task_4_2),
                Group {
                    groupTask_4_3->executeMode(),
                    Workflow(groupTask_4_3->workflowPolicy()),
                    OnGroupSetup([groupTask_4_3] { groupTask_4_3->setState(State::Running); }),
                    OnGroupDone([groupTask_4_3] { groupTask_4_3->setState(State::Done); }),
                    OnGroupError([groupTask_4_3] { groupTask_4_3->setState(State::Error); }),

                    taskItem(task_4_3_1),
                    taskItem(task_4_3_2),
                    taskItem(task_4_3_3),
                    taskItem(task_4_3_4)
                },
                taskItem(task_4_4),
                taskItem(task_4_5)
            },
            taskItem(task_5)
        };
        return root;
    };

    // Non-task GUI handling

    auto createTaskTree = [&] {
        TaskTree *taskTree = new TaskTree(treeRoot());
        progressBar->setMaximum(taskTree->progressMaximum());
        QObject::connect(taskTree, &TaskTree::progressValueChanged,
                         progressBar, &QProgressBar::setValue);
        return taskTree;
    };

    auto stopTaskTree = [&] {
        if (taskTree)
            taskTree->stop();
        // TODO: unlock GUI controls?
    };

    auto resetTaskTree = [&] {
        if (!taskTree)
            return;

        stopTaskTree();
        taskTree.reset();
        for (StateWidget *widget : allStateWidgets)
            widget->setState(State::Initial);
        progressBar->setValue(0);
    };

    auto startTaskTree = [&] {
        resetTaskTree();
        taskTree.reset(createTaskTree());
        taskTree->start();
        // TODO: lock GUI controls?
    };

    QObject::connect(startButton, &QAbstractButton::clicked, startTaskTree);
    QObject::connect(stopButton, &QAbstractButton::clicked, stopTaskTree);
    QObject::connect(resetButton, &QAbstractButton::clicked, resetTaskTree);

    // Hack in order to show initial size minimal, but without scrollbars.
    // Apparently setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow) doesn't work.
    const int margin = 2;
    scrollArea->setMinimumSize(scrollAreaWidget->minimumSizeHint().grownBy({0, 0, margin, margin}));
    QTimer::singleShot(0, scrollArea, [&] { scrollArea->setMinimumSize({0, 0}); });

    mainWidget.show();

    return app.exec();
}
