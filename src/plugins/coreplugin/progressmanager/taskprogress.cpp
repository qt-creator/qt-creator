// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "taskprogress.h"

#include "futureprogress.h"
#include "progressmanager.h"

#include <solutions/tasking/tasktree.h>

#include <utils/mathutils.h>
#include <utils/qtcassert.h>

#include <QFutureWatcher>
#include <QPointer>
#include <QTimer>

using namespace Utils;
using namespace Tasking;

namespace Core {

static const int ProgressResolution = 100; // 100 discrete values
static const int TimerInterval = 20; // 20 ms = 50 Hz

class TaskProgressPrivate : public QObject
{
public:
    explicit TaskProgressPrivate(TaskProgress *progress, TaskTree *taskTree);
    ~TaskProgressPrivate();

    void advanceTick();
    void advanceProgress(int newValue);
    void updateProgress();

    int m_currentTick = 0;

    int m_currentProgress = 0; // from TaskTree (max value = task count)

    TaskTree *m_taskTree = nullptr;
    QTimer m_timer;
    QFutureWatcher<void> m_watcher;
    QFutureInterface<void> m_futureInterface;
    QPointer<FutureProgress> m_futureProgress;
    Id m_id;
    bool m_isAutoStopOnCancel = true;
    int m_halfLifeTimePerTask = 1000; // 1000 ms
    QString m_displayName;
    FutureProgress::KeepOnFinishType m_keep = FutureProgress::HideOnFinish;
    bool m_isSubtitleVisibleInStatusBar = false;
    QString m_subtitle;
};

TaskProgressPrivate::TaskProgressPrivate(TaskProgress *progress, TaskTree *taskTree)
    : QObject(progress)
    , m_taskTree(taskTree)
    , m_timer(progress)
{
    m_timer.setInterval(TimerInterval);
    connect(&m_timer, &QTimer::timeout, &m_timer, [this] { advanceTick(); });
}

TaskProgressPrivate::~TaskProgressPrivate()
{
    if (m_futureInterface.isRunning()) {
        m_futureInterface.reportCanceled();
        m_futureInterface.reportFinished();
    }
}

void TaskProgressPrivate::advanceTick()
{
    ++m_currentTick;
    updateProgress();
}

void TaskProgressPrivate::advanceProgress(int newValue)
{
    m_currentProgress = newValue;
    m_currentTick = 0;
    updateProgress();
}

void TaskProgressPrivate::updateProgress()
{
    const int halfLife = qRound(double(m_halfLifeTimePerTask) / TimerInterval);
    const int pMin = ProgressResolution * m_currentProgress;
    const int pMax = ProgressResolution * (m_currentProgress + 1);
    const int newValue = MathUtils::interpolateExponential(m_currentTick, halfLife, pMin, pMax);
    m_futureInterface.setProgressValue(newValue);
}

/*!
    \class Core::TaskProgress
    \inmodule QtCreator

    \brief The TaskProgress class is responsible for showing progress of the running task tree.

    It's possible to cancel the running task tree automatically after pressing a small 'x'
    indicator on progress panel. In this case TaskTree::stop() method is being called.
*/

TaskProgress::TaskProgress(TaskTree *taskTree)
    : QObject(taskTree)
    , d(new TaskProgressPrivate(this, taskTree))
{
    connect(&d->m_watcher, &QFutureWatcher<void>::canceled, this, [this] {
        emit canceled();
        if (d->m_isAutoStopOnCancel)
            d->m_taskTree->stop();
    });
    connect(d->m_taskTree, &TaskTree::started, this, [this] {
        d->m_futureInterface = QFutureInterface<void>();
        d->m_futureInterface.setProgressRange(
                    0, d->m_taskTree->progressMaximum() * ProgressResolution);
        d->m_watcher.setFuture(d->m_futureInterface.future());
        d->m_futureInterface.reportStarted();
        d->advanceProgress(0);

        const Id id = d->m_id.isValid() ? d->m_id : Id::fromString(d->m_displayName + ".action");
        d->m_futureProgress = ProgressManager::addTask(d->m_futureInterface.future(),
                                                       d->m_displayName, id);
        d->m_futureProgress->setKeepOnFinish(d->m_keep);
        d->m_futureProgress->setSubtitleVisibleInStatusBar(d->m_isSubtitleVisibleInStatusBar);
        d->m_futureProgress->setSubtitle(d->m_subtitle);
        d->m_timer.start();
    });
    connect(d->m_taskTree, &TaskTree::progressValueChanged, this, [this](int value) {
        d->advanceProgress(value);
    });
    connect(d->m_taskTree, &TaskTree::done, this, [this] {
        d->m_timer.stop();
        d->m_futureInterface.reportFinished();
    });
    connect(d->m_taskTree, &TaskTree::errorOccurred, this, [this] {
        d->m_timer.stop();
        d->m_futureInterface.reportCanceled();
        d->m_futureInterface.reportFinished();
    });
}

TaskProgress::~TaskProgress() = default;

void TaskProgress::setId(Utils::Id id)
{
    d->m_id = id;
}

void TaskProgress::setAutoStopOnCancel(bool enable)
{
    d->m_isAutoStopOnCancel = enable;
}

void TaskProgress::setHalfLifeTimePerTask(int msecs)
{
    d->m_halfLifeTimePerTask = msecs;
}

void TaskProgress::setDisplayName(const QString &name)
{
    d->m_displayName = name;
    if (d->m_futureProgress)
        d->m_futureProgress->setTitle(d->m_displayName);
}

void TaskProgress::setKeepOnFinish(FutureProgress::KeepOnFinishType keepType)
{
    d->m_keep = keepType;
    if (d->m_futureProgress)
        d->m_futureProgress->setKeepOnFinish(d->m_keep);
}

void TaskProgress::setSubtitleVisibleInStatusBar(bool visible)
{
    d->m_isSubtitleVisibleInStatusBar = visible;
    if (d->m_futureProgress)
        d->m_futureProgress->setSubtitleVisibleInStatusBar(d->m_isSubtitleVisibleInStatusBar);
}

void TaskProgress::setSubtitle(const QString &subtitle)
{
    d->m_subtitle = subtitle;
    if (d->m_futureProgress)
        d->m_futureProgress->setSubtitle(d->m_subtitle);
}

} // namespace Core
