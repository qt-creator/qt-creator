// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "taskwidget.h"

#include <tasking/tasktree.h>

#include <QApplication>
#include <QBoxLayout>
#include <QGroupBox>
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

QWidget *taskGroup(QWidget *groupWidget, const QList<QWidget *> &widgets)
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

    groupTask_1->setWorkflowPolicy(WorkflowPolicy::ContinueOnDone);
    groupTask_4->setWorkflowPolicy(WorkflowPolicy::FinishAllAndDone);
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
    }

    // Task tree (takes initial configuation from GUI)

    std::unique_ptr<TaskTree> taskTree;

    const auto createTask = [](TaskWidget *widget) {
        const milliseconds timeout{widget->busyTime() * 1000};
        const auto setupTask = [widget, timeout](milliseconds &taskObject) {
            taskObject = timeout;
            widget->setState(State::Running);
        };
        const Group root {
            widget->isSuccess() ? stopOnError : finishAllAndError,
            TimeoutTask(setupTask),
            onGroupDone([widget] { widget->setState(State::Done); }),
            onGroupError([widget] { widget->setState(State::Error); })
        };
        return root;
    };

    const auto recipe = [&] {
        const Group root {
            rootGroup->executeMode(),
            rootGroup->workflowPolicy(),
            onGroupSetup([rootGroup] { rootGroup->setState(State::Running); }),
            onGroupDone([rootGroup] { rootGroup->setState(State::Done); }),
            onGroupError([rootGroup] { rootGroup->setState(State::Error); }),

            Group {
                groupTask_1->executeMode(),
                groupTask_1->workflowPolicy(),
                onGroupSetup([groupTask_1] { groupTask_1->setState(State::Running); }),
                onGroupDone([groupTask_1] { groupTask_1->setState(State::Done); }),
                onGroupError([groupTask_1] { groupTask_1->setState(State::Error); }),

                createTask(task_1_1),
                createTask(task_1_2),
                createTask(task_1_3)
            },
            createTask(task_2),
            createTask(task_3),
            Group {
                groupTask_4->executeMode(),
                groupTask_4->workflowPolicy(),
                onGroupSetup([groupTask_4] { groupTask_4->setState(State::Running); }),
                onGroupDone([groupTask_4] { groupTask_4->setState(State::Done); }),
                onGroupError([groupTask_4] { groupTask_4->setState(State::Error); }),

                createTask(task_4_1),
                createTask(task_4_2),
                Group {
                    groupTask_4_3->executeMode(),
                    groupTask_4_3->workflowPolicy(),
                    onGroupSetup([groupTask_4_3] { groupTask_4_3->setState(State::Running); }),
                    onGroupDone([groupTask_4_3] { groupTask_4_3->setState(State::Done); }),
                    onGroupError([groupTask_4_3] { groupTask_4_3->setState(State::Error); }),

                    createTask(task_4_3_1),
                    createTask(task_4_3_2),
                    createTask(task_4_3_3),
                    createTask(task_4_3_4)
                },
                createTask(task_4_4),
                createTask(task_4_5)
            },
            createTask(task_5)
        };
        return root;
    };

    // Non-task GUI handling

    const auto createTaskTree = [&] {
        TaskTree *taskTree = new TaskTree(recipe());
        progressBar->setMaximum(taskTree->progressMaximum());
        QObject::connect(taskTree, &TaskTree::progressValueChanged,
                         progressBar, &QProgressBar::setValue);
        return taskTree;
    };

    const auto stopTaskTree = [&] {
        if (taskTree)
            taskTree->stop();
        // TODO: unlock GUI controls?
    };

    const auto resetTaskTree = [&] {
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
    const int margin = 4;
    scrollArea->setMinimumSize(scrollAreaWidget->minimumSizeHint().grownBy({0, 0, margin, margin}));
    QTimer::singleShot(0, scrollArea, [&] { scrollArea->setMinimumSize({0, 0}); });

    mainWidget.show();

    return app.exec();
}
