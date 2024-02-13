// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "taskwidget.h"

#include <tasking/tasktree.h>
#include <tasking/tasktreerunner.h>

#include <QApplication>
#include <QBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>

using namespace Tasking;

using namespace std::chrono;

static QWidget *hr()
{
    auto frame = new QFrame;
    frame->setFrameShape(QFrame::HLine);
    frame->setFrameShadow(QFrame::Sunken);
    return frame;
}

static QWidget *taskGroup(QWidget *groupWidget, const QList<QWidget *> &widgets)
{
    QWidget *widget = new QWidget;
    QBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(groupWidget);
    QGroupBox *groupBox = new QGroupBox;
    QBoxLayout *subLayout = new QVBoxLayout(groupBox);
    for (int i = 0; i < widgets.size(); ++i) {
        if (i > 0)
            subLayout->addWidget(hr());
        subLayout->addWidget(widgets.at(i));
    }
    layout->addWidget(groupBox);
    return widget;
}

static State resultToState(DoneWith result)
{
    switch (result) {
    case DoneWith::Success: return State::Success;
    case DoneWith::Error: return State::Error;
    case DoneWith::Cancel: return State::Canceled;
    }
    return State::Initial;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget mainWidget;
    mainWidget.setWindowTitle("Task Tree Demo");

    // Non-task GUI

    QToolButton *startButton = new QToolButton;
    startButton->setText("Start");
    QToolButton *stopButton = new QToolButton;
    stopButton->setText("Stop");
    QToolButton *resetButton = new QToolButton;
    resetButton->setText("Reset");
    QProgressBar *progressBar = new QProgressBar;
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    QWidget *scrollAreaWidget = new QWidget;

    // Task GUI

    QList<GroupWidget *> allGroupWidgets;
    QList<TaskWidget *> allTaskWidgets;

    auto createGroupWidget = [&allGroupWidgets] {
        auto *widget = new GroupWidget;
        allGroupWidgets.append(widget);
        return widget;
    };
    auto createTaskWidget = [&allTaskWidgets] {
        auto *widget = new TaskWidget;
        allTaskWidgets.append(widget);
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
    task_1_2->setDesiredResult(DoneResult::Error);
    task_1_3->setBusyTime(3);
    task_4_3_1->setBusyTime(4);
    task_4_3_2->setBusyTime(2);
    task_4_3_3->setBusyTime(1);
    task_4_3_4->setBusyTime(3);
    task_4_3_4->setDesiredResult(DoneResult::Error);
    task_4_4->setBusyTime(6);
    task_4_4->setBusyTime(3);

    groupTask_1->setWorkflowPolicy(WorkflowPolicy::ContinueOnSuccess);
    groupTask_4->setWorkflowPolicy(WorkflowPolicy::FinishAllAndSuccess);
    groupTask_4_3->setExecuteMode(ExecuteMode::Parallel);
    groupTask_4_3->setWorkflowPolicy(WorkflowPolicy::StopOnError);

    // Task layout

    {
        QWidget *taskTree = taskGroup(rootGroup, {
            taskGroup(groupTask_1, {
                task_1_1,
                task_1_2,
                task_1_3
            }),
            task_2,
            task_3,
            taskGroup(groupTask_4, {
                task_4_1,
                task_4_2,
                taskGroup(groupTask_4_3, {
                    task_4_3_1,
                    task_4_3_2,
                    task_4_3_3,
                    task_4_3_4,
                }),
                task_4_4,
                task_4_5
            }),
            task_5
        });
        QBoxLayout *scrollLayout = new QVBoxLayout(scrollAreaWidget);
        scrollLayout->addWidget(taskTree);
        scrollLayout->addStretch();
        scrollArea->setWidget(scrollAreaWidget);

        QBoxLayout *mainLayout = new QVBoxLayout(&mainWidget);
        QBoxLayout *subLayout = new QHBoxLayout;
        subLayout->addWidget(startButton);
        subLayout->addWidget(stopButton);
        subLayout->addWidget(resetButton);
        subLayout->addWidget(progressBar);
        mainLayout->addLayout(subLayout);
        mainLayout->addWidget(hr());
        mainLayout->addWidget(scrollArea);
        mainLayout->addWidget(hr());
        QBoxLayout *footerLayout = new QHBoxLayout;
        footerLayout->addWidget(new StateLabel(State::Initial));
        footerLayout->addWidget(new StateLabel(State::Running));
        footerLayout->addWidget(new StateLabel(State::Success));
        footerLayout->addWidget(new StateLabel(State::Error));
        footerLayout->addWidget(new StateLabel(State::Canceled));
        mainLayout->addLayout(footerLayout);
    }

    // Task tree (takes initial configuation from GUI)

    TaskTreeRunner taskTreeRunner;

    QObject::connect(&taskTreeRunner, &TaskTreeRunner::aboutToStart,
                     progressBar, [progressBar](TaskTree *taskTree) {
        progressBar->setMaximum(taskTree->progressMaximum());
        QObject::connect(taskTree, &TaskTree::progressValueChanged,
                         progressBar, &QProgressBar::setValue);
    });

    const auto setupGroup = [](GroupWidget *widget) {
        return GroupItem {
            widget->executeMode(),
            widget->workflowPolicy(),
            onGroupSetup([widget] { widget->setState(State::Running); }),
            onGroupDone([widget](DoneWith result) { widget->setState(resultToState(result)); }),
        };
    };

    const auto setupTask = [](TaskWidget *widget) {
        const milliseconds timeout(widget->busyTime() * 1000);
        const auto onSetup = [widget, timeout](milliseconds &taskObject) {
            taskObject = timeout;
            widget->setState(State::Running);
        };
        const auto onDone = [widget, desiredResult = widget->desiredResult()](DoneWith doneWith) {
            // The TimeoutTask, when not DoneWith::Cancel, always reports DoneWith::Success.
            // Tweak the final result in case the desired result is Error.
            const DoneWith result = doneWith == DoneWith::Success
                            && desiredResult == DoneResult::Error ? DoneWith::Error : doneWith;
            widget->setState(resultToState(result));
            return desiredResult;
        };
        return TimeoutTask(onSetup, onDone);
    };

    const auto recipe = [&] {
        const Group root {
            setupGroup(rootGroup),
            Group {
                setupGroup(groupTask_1),
                setupTask(task_1_1),
                setupTask(task_1_2),
                setupTask(task_1_3)
            },
            setupTask(task_2),
            setupTask(task_3),
            Group {
                setupGroup(groupTask_4),
                setupTask(task_4_1),
                setupTask(task_4_2),
                Group {
                    setupGroup(groupTask_4_3),
                    setupTask(task_4_3_1),
                    setupTask(task_4_3_2),
                    setupTask(task_4_3_3),
                    setupTask(task_4_3_4)
                },
                setupTask(task_4_4),
                setupTask(task_4_5)
            },
            setupTask(task_5)
        };
        return root;
    };

    // Non-task GUI handling

    const auto stopTaskTree = [&] {
        if (taskTreeRunner.isRunning())
            taskTreeRunner.cancel();
    };

    const auto resetTaskTree = [&] {
        taskTreeRunner.reset();
        for (GroupWidget *widget : allGroupWidgets)
            widget->setState(State::Initial);
        for (TaskWidget *widget : allTaskWidgets)
            widget->setState(State::Initial);
        progressBar->setValue(0);
    };

    auto startTaskTree = [&] {
        resetTaskTree();
        taskTreeRunner.start(recipe());
    };

    QObject::connect(startButton, &QAbstractButton::clicked, startTaskTree);
    QObject::connect(stopButton, &QAbstractButton::clicked, stopTaskTree);
    QObject::connect(resetButton, &QAbstractButton::clicked, resetTaskTree);

    // Hack in order to show initial size minimal, but without scrollbars.
    // Apparently setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow) doesn't work.
    const int margin = 4;
    scrollArea->setMinimumSize(scrollAreaWidget->minimumSizeHint().grownBy({0, 0, margin, margin}));
    QTimer::singleShot(0, scrollArea, [&] { scrollArea->setMinimumSize({0, 0}); });

    mainWidget.show();

    return app.exec();
}
