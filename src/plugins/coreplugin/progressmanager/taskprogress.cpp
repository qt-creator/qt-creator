// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "taskprogress.h"

#include "progressmanager.h"

#include <utils/qtcassert.h>
#include <utils/tasktree.h>

#include <QFutureWatcher>

using namespace Utils;

namespace Core {

class TaskProgressPrivate : public QObject
{
public:
    explicit TaskProgressPrivate(TaskProgress *progress, TaskTree *taskTree);
    ~TaskProgressPrivate();

    TaskTree *m_taskTree = nullptr;
    QFutureWatcher<void> m_watcher;
    QFutureInterface<void> m_futureInterface;
    QString m_displayName;
};

TaskProgressPrivate::TaskProgressPrivate(TaskProgress *progress, TaskTree *taskTree)
    : QObject(progress)
    , m_taskTree(taskTree)
{
}

TaskProgressPrivate::~TaskProgressPrivate()
{
    if (m_futureInterface.isRunning()) {
        m_futureInterface.reportCanceled();
        m_futureInterface.reportFinished();
        // TODO: should we stop the process? Or just mark the process canceled?
        // What happens to task in progress manager?
    }
}

/*!
    \class Core::TaskProgress

    \brief The TaskProgress class is responsible for showing progress of the running task tree.

    It's possible to cancel the running task tree automatically after pressing a small 'x'
    indicator on progress panel. In this case TaskTree::stop() method is being called.
*/

TaskProgress::TaskProgress(TaskTree *taskTree)
    : QObject(taskTree)
    , d(new TaskProgressPrivate(this, taskTree))
{
    connect(&d->m_watcher, &QFutureWatcher<void>::canceled, this, [this] {
        d->m_taskTree->stop(); // TODO: should we have different cancel policies?
    });
    connect(d->m_taskTree, &TaskTree::started, this, [this] {
        d->m_futureInterface = QFutureInterface<void>();
        d->m_futureInterface.setProgressRange(0, d->m_taskTree->progressMaximum());
        d->m_watcher.setFuture(d->m_futureInterface.future());
        d->m_futureInterface.reportStarted();

        const auto id = Id::fromString(d->m_displayName + ".action");
        ProgressManager::addTask(d->m_futureInterface.future(), d->m_displayName, id);
    });
    connect(d->m_taskTree, &TaskTree::progressValueChanged, this, [this](int value) {
        d->m_futureInterface.setProgressValue(value);
    });
    connect(d->m_taskTree, &TaskTree::done, this, [this] {
        d->m_futureInterface.reportFinished();
    });
    connect(d->m_taskTree, &TaskTree::errorOccurred, this, [this] {
        d->m_futureInterface.reportCanceled();
        d->m_futureInterface.reportFinished();
    });
}

void TaskProgress::setDisplayName(const QString &name)
{
    d->m_displayName = name;
}

} // namespace Core
