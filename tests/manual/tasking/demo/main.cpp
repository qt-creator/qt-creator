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

struct GroupSetup
{
    WorkflowPolicy policy = WorkflowPolicy::StopOnError;
    ExecuteMode mode = ExecuteMode::Sequential;
};

class GlueItem
{
public:
    virtual ExecutableItem recipe() const = 0;
    virtual QWidget *widget() const = 0;
    virtual void reset() const = 0;

    ~GlueItem() { qDeleteAll(m_children); }

protected:
    GlueItem(const QList<GlueItem *> children) : m_children(children) {}

    void resetChildren() const
    {
        for (GlueItem *child : m_children)
            child->reset();
    }

    QList<GroupItem> childrenRecipes() const
    {
        QList<GroupItem> recipes;
        for (GlueItem *child : m_children)
            recipes.append(child->recipe());
        return recipes;
    }

private:
    QList<GlueItem *> m_children;
};

class GroupGlueItem : public GlueItem
{
public:
    GroupGlueItem(const GroupSetup &setup, const QList<GlueItem *> &children)
        : GlueItem(children)
        , m_groupWidget(new GroupWidget)
        , m_widget(new QWidget)
    {
        QBoxLayout *layout = new QHBoxLayout(m_widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_groupWidget);
        QGroupBox *groupBox = new QGroupBox;
        QBoxLayout *subLayout = new QVBoxLayout(groupBox);
        for (int i = 0; i < children.size(); ++i) {
            if (i > 0)
                subLayout->addWidget(hr());
            subLayout->addWidget(children.at(i)->widget());
        }
        layout->addWidget(groupBox);
        m_groupWidget->setWorkflowPolicy(setup.policy);
        m_groupWidget->setExecuteMode(setup.mode);
    }

    ExecutableItem recipe() const final
    {
        return Group {
            m_groupWidget->executeMode(),
            m_groupWidget->workflowPolicy(),
            onGroupSetup([this] { m_groupWidget->setState(State::Running); }),
            childrenRecipes(),
            onGroupDone([this](DoneWith result) { m_groupWidget->setState(resultToState(result)); })
        };
    }
    QWidget *widget() const final { return m_widget; }
    void reset() const final
    {
        m_groupWidget->setState(State::Initial);
        resetChildren();
    }

private:
    GroupWidget *m_groupWidget = nullptr;
    QWidget *m_widget = nullptr;
};

class TaskGlueItem : public GlueItem
{
public:
    TaskGlueItem(int busyTime, DoneResult result)
        : GlueItem({})
        , m_taskWidget(new TaskWidget)
    {
        m_taskWidget->setBusyTime(busyTime);
        m_taskWidget->setDesiredResult(result);
    }

    ExecutableItem recipe() const final
    {
        const milliseconds timeout(m_taskWidget->busyTime() * 1000);
        const auto onSetup = [this, timeout](milliseconds &taskObject) {
            taskObject = timeout;
            m_taskWidget->setState(State::Running);
        };
        const auto onDone = [this, desiredResult = m_taskWidget->desiredResult()](DoneWith doneWith) {
            // The TimeoutTask, when not DoneWith::Cancel, always reports DoneWith::Success.
            // Tweak the final result in case the desired result is Error.
            const DoneWith result = doneWith == DoneWith::Success
                           && desiredResult == DoneResult::Error ? DoneWith::Error : doneWith;
            m_taskWidget->setState(resultToState(result));
            return desiredResult;
        };
        return TimeoutTask(onSetup, onDone);
    }

    QWidget *widget() const final { return m_taskWidget; }
    void reset() const final { m_taskWidget->setState(State::Initial); }

private:
    TaskWidget *m_taskWidget = nullptr;
};

static GlueItem *group(const GroupSetup &groupSetup, const QList<GlueItem *> children)
{
    return new GroupGlueItem(groupSetup, children);
}

static GlueItem *task(int busyTime = 1, DoneResult result = DoneResult::Success)
{
    return new TaskGlueItem(busyTime, result);
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

    std::unique_ptr<GlueItem> tree {
        group({WorkflowPolicy::ContinueOnSuccess}, {
            group({}, {
                task(),
                task(2, DoneResult::Error),
                task(3)
            }),
            task(),
            task(),
            group({WorkflowPolicy::FinishAllAndSuccess}, {
                task(),
                task(),
                group({WorkflowPolicy::StopOnError, ExecuteMode::Parallel}, {
                    task(4),
                    task(2),
                    task(1),
                    task(3),
                }),
                task(2),
                task(3)
            }),
            task()
        })
    };

    {
        // Task layout
        QBoxLayout *scrollLayout = new QVBoxLayout(scrollAreaWidget);
        scrollLayout->addWidget(tree->widget());
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

    TaskTreeRunner taskTreeRunner;

    QObject::connect(&taskTreeRunner, &TaskTreeRunner::aboutToStart,
                     progressBar, [progressBar](TaskTree *taskTree) {
        progressBar->setMaximum(taskTree->progressMaximum());
        QObject::connect(taskTree, &TaskTree::progressValueChanged,
                         progressBar, &QProgressBar::setValue);
    });

    const auto stopTaskTree = [&] {
        if (taskTreeRunner.isRunning())
            taskTreeRunner.cancel();
    };

    const auto resetTaskTree = [&] {
        taskTreeRunner.reset();
        tree->reset();
        progressBar->setValue(0);
    };

    auto startTaskTree = [&] {
        resetTaskTree();
        taskTreeRunner.start({tree->recipe()});
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
