// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QTASKTREE_QBARRIERTASK_H
#define QTASKTREE_QBARRIERTASK_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/QSharedData>

QT_BEGIN_NAMESPACE

namespace QtTaskTree { class WhenPrivate; }

QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::WhenPrivate)

namespace QtTaskTree {

class QBarrierPrivate;

class Q_TASKTREE_EXPORT QBarrier : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QBarrier)

public:
    QBarrier() : QBarrier(nullptr) {}
    explicit QBarrier(QObject *parent);
    ~QBarrier() override;

    void setLimit(qsizetype value);
    qsizetype limit() const;

    void start();
    void advance();
    void stopWithResult(DoneResult result);

    bool isRunning() const;
    qsizetype current() const;
    std::optional<DoneResult> result() const;

Q_SIGNALS:
    void done(QtTaskTree::DoneResult success, QPrivateSignal);

protected:
    bool event(QEvent *event) override;
};

using QBarrierTask = QCustomTask<QBarrier>;

class Q_TASKTREE_EXPORT QStartedBarrier : public QBarrier
{
    Q_OBJECT

public:
    QStartedBarrier() : QStartedBarrier(nullptr) {}
    explicit QStartedBarrier(QObject *parent);
    explicit QStartedBarrier(qsizetype limit, QObject *parent = nullptr);

    ~QStartedBarrier() override;

protected:
    bool event(QEvent *event) override;
};

using QStoredBarrier = Storage<QStartedBarrier>;

Q_TASKTREE_EXPORT ExecutableItem barrierAwaiterTask(const QStoredBarrier &storedBarrier);

template <typename Signal>
ExecutableItem signalAwaiterTask(const typename QtPrivate::FunctionPointer<Signal>::Object *sender,
                                 Signal signal)
{
    return QBarrierTask([sender, signal](QBarrier &barrier) {
        QObject::connect(sender, signal, &barrier, &QBarrier::advance, Qt::SingleShotConnection);
    });
}

using BarrierKickerGetter = std::function<ExecutableItem(const QStoredBarrier &)>;

class When final
{
public:
    Q_TASKTREE_EXPORT explicit When(const BarrierKickerGetter &kicker,
                                    WorkflowPolicy policy = WorkflowPolicy::StopOnError);

    template <typename Task, typename Adapter, typename Deleter, typename Signal>
    explicit When(const QCustomTask<Task, Adapter, Deleter> &customTask, Signal signal,
                  WorkflowPolicy policy = WorkflowPolicy::StopOnError)
        : When(kickerForSignal(customTask, signal), policy)
    {}

    QT_TASKTREE_DECLARE_SMFS(When, Q_TASKTREE_EXPORT)

private:
    Q_TASKTREE_EXPORT friend Group operator>>(const When &whenItem, const Do &doItem);

    template <typename Task, typename Adapter, typename Deleter, typename Signal>
    static BarrierKickerGetter kickerForSignal(const QCustomTask<Task, Adapter, Deleter> &task,
                                               Signal signal)
    {
        return [taskHandler = task.taskHandler(), signal](const QStoredBarrier &barrier) {
            auto handler = std::move(taskHandler);
            const auto wrappedSetupHandler
                = [originalSetupHandler = std::move(handler.m_taskAdapterSetupHandler),
                   barrier, signal](void *taskAdapter) {
                const SetupResult setupResult = std::invoke(originalSetupHandler, taskAdapter);
                using TaskAdapter = typename QCustomTask<Task, Adapter, Deleter>::TaskAdapter;
                TaskAdapter *adapter = static_cast<TaskAdapter *>(taskAdapter);
                QObject::connect(adapter->task.get(), signal,
                                 barrier.activeStorage(), &QBarrier::advance);
                return setupResult;
            };
            handler.m_taskAdapterSetupHandler = std::move(wrappedSetupHandler);
            return ExecutableItem(std::move(handler));
        };
    }

    QExplicitlySharedDataPointer<WhenPrivate> d;
};

} // namespace QtTaskTree

QT_END_NAMESPACE

#endif // QTASKTREE_QBARRIERTASK_H
