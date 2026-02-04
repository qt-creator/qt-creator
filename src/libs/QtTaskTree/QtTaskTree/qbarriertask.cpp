// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTaskTree/qbarriertask.h>

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

using namespace QtTaskTree;

#define QT_TASKTREE_STRING(cond) qDebug("SOFT ASSERT: \"%s\" in %s: %s", cond,  __FILE__, QT_STRINGIFY(__LINE__))
#define QT_TASKTREE_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QT_TASKTREE_STRING(#cond); action; } do {} while (0)

class QBarrierPrivate : public QObjectPrivate
{
public:
    std::optional<QtTaskTree::DoneResult> m_result;
    int m_limit = 1;
    int m_current = -1;
};

/*!
    \class QBarrier
    \inheaderfile qbarriertask.h
    \inmodule TaskTree
    \brief An asynchronous task that finishes on demand.
    \reentrant

    QBarrier waits for subsequent calls to \l advance() to reach
    the barrier's limit (1 by default). It finishes with
    \l {QtTaskTree::} {DoneResult::Success}.

    It's often used in QTaskTree recipes to hold the execution of subsequent
    sequential tasks until some other running task delivers
    some data which are needed to setup the subsequent tasks.
*/

/*!
    Constructs a QBarrier with a given \a parent.
*/
QBarrier::QBarrier(QObject *parent)
    : QObject(*new QBarrierPrivate, parent)
{}

QBarrier::~QBarrier() = default;

/*!
    Sets the limit to \a value. After it is started, the barrier finishes
    when the number of calls to \l advance() reaches the limit.
*/
void QBarrier::setLimit(int value)
{
    QT_TASKTREE_ASSERT(!isRunning(), return);
    QT_TASKTREE_ASSERT(value > 0, return);

    d_func()->m_limit = value;
}

/*!
    Returns the current limit of the barrier.
*/
int QBarrier::limit() const
{
    return d_func()->m_limit;
}

/*!
    Starts the barrier.
*/
void QBarrier::start()
{
    QT_TASKTREE_ASSERT(!isRunning(), return);
    d_func()->m_current = 0;
    d_func()->m_result.reset();
}

/*!
    Advances the barrier. If the number of calls to advance() reaches
    the barrier's limit, the barrier finishes with
    \l {QtTaskTree::} {DoneResult::Success}.
*/
void QBarrier::advance()
{
    // Calling advance on finished is OK
    QT_TASKTREE_ASSERT(isRunning() || d_func()->m_result, return);
    if (!isRunning()) // Can't advance not running barrier
        return;
    ++d_func()->m_current;
    if (d_func()->m_current == d_func()->m_limit)
        stopWithResult(DoneResult::Success);
}

/*!
    Stops the running barrier unconditionally with a given \a result.
    The barrier's limit is ignored.
*/
void QBarrier::stopWithResult(DoneResult result)
{
    // Calling stopWithResult on finished is OK
    QT_TASKTREE_ASSERT(isRunning() || d_func()->m_result, return);
    if (!isRunning()) // Can't advance not running barrier
        return;
    d_func()->m_current = -1;
    d_func()->m_result = result;
    Q_EMIT done(result, QPrivateSignal());
}

/*!
    Returns \c true if the barrier is currently running;
    otherwise returns \c false.
*/
bool QBarrier::isRunning() const
{
    return d_func()->m_current >= 0;
}

/*!
    Returns the current advance count of the barrier.
*/
int QBarrier::current() const
{
    return d_func()->m_current;
}

/*!
    Returns the result of barrier execution. If barrier wasn't started
    or it's still running, the returned optional is empty.
    Otherwise, it returns the result of the last execution.
*/
std::optional<DoneResult> QBarrier::result() const
{
    return d_func()->m_result;
}

/*!
    \fn void QBarrier::done(QtTaskTree::DoneResult result)

    This signal is emitted when the barrier finished,
    passing the final \a result of the execution.
*/

/*!
    \typedef QBarrierTask
    \relates QCustomTask

    Type alias for the QCustomTask<QBarrier>, to be used inside recipes.
*/

/*!
    \class QStartedBarrier
    \inheaderfile qbarriertask.h
    \inmodule TaskTree
    \brief A started QBarrier with a given Limit.
    \reentrant

    QStartedBarrier is a QBarrier with a given Limit,
    already started when constucted.
*/

/*!
    \fn template <int Limit = 1> QStartedBarrier<Limit>::QStartedBarrier(QObject *parent = nullptr)

    Creates started QBarrier with a given \a parent and Limit.
    The default Limit is 1.
*/

/*!
    \typedef QStoredMultiBarrier
    \relates QStartedBarrier

    Type alias for the QtTaskTree::Storage<QStartedBarrier<Limit>>,
    to be used inside recipes.
*/

/*!
    \typedef QStoredBarrier
    \relates QStartedBarrier

    Type alias for the QStoredMultiBarrier<1>, to be used inside recipes.
*/

namespace QtTaskTree {

/*!
    \fn template <int Limit> ExecutableItem barrierAwaiterTask(const QStoredMultiBarrier<Limit> &storedBarrier)

    Returns the awaiter task that finishes when passed \a storedBarrier
    is finished.

    \note The passed \a storedBarrier needs to be placed in the same recipe
          as a sibling item to the returned task or in any ancestor \l Group,
          otherwise you may expect a crash.
*/

/*!
    \fn template <typename Signal> ExecutableItem signalAwaiterTask(const typename QtPrivate::FunctionPointer<Signal>::Object *sender, Signal signal)

    Returns the awaiter task that finishes when passed \a sender emits
    the \a signal.

    \note The connection to the passed \a sender and \a signal is established
          not when the returned task is created or placed into a recipe,
          but when the task is started by the QTaskTree. Ensure, the passed
          \a sender outlives any running task tree containing the returned task.
          If the \a signal was emitted before the task started, i.e. before
          the connection was established, the task will finish on first
          emission of the \a signal after the task started.
*/

/*!
    \typedef QtTaskTree::BarrierKickerGetter
    \relates QStartedBarrier

    Type alias for the function taking a QStoredBarrier and returning
    \l {QtTaskTree::} {ExecutableItem}, i.e.
    \c std::function<ExecutableItem(const QStoredBarrier &)>,
    to be used inside \l {QtTaskTree::} {When} constructor.
*/

class WhenPrivate : public QSharedData
{
public:
    WhenPrivate(const BarrierKickerGetter &kicker, WorkflowPolicy policy)
        : m_barrierKicker(kicker)
        , m_workflowPolicy(policy)
    {}
    BarrierKickerGetter m_barrierKicker;
    WorkflowPolicy m_workflowPolicy = WorkflowPolicy::StopOnError;
};

/*!
    \class QtTaskTree::When
    \inheaderfile qbarriertask.h
    \inmodule TaskTree
    \brief An element delaying the execution of a body until barrier advance.
    \reentrant

    When is used to delay the execution of a \l Do body until related
    QStoredBarrier is advanced or until QCustomTask's signal is emitted.
    It is used in conjunction with \l Do element like: \c {When () >> Do {}}.

    \sa Do
*/

/*!
    Creates a delaying element, taking \a kicker returning ExecutableItem
    to be run in parallel with a \l Do body. The \l Do body is delayed
    until the QStoredBarrier passed to the \a kicker is advanced.
    The ExecutableItem returned by the \a kicker and the \l Do body
    will run in parallel using the \a policy.

    For example, if you want to delay the execution of the subsequent tasks
    until the QProcess is started, the recipe could look like:

    \code
        const auto kicker = [](const QStoredBarrier &barrier) {
            const auto onSetup = [barrier](QProcess &process) {
                QObject::connect(&process, &QProcess::started, barrier.activeStorage(), &QBarrier::advance);
                ... // Setup process program, arguments, environment, etc...
            };
            return QProcessTask(onSetup);
        };

        const Group recipe {
            When (kicker) >> Do {
                delayedTask1,
                ...
            }
        };
    \endcode

    When the above recipe is executed, the QTaskTree runs a QProcessTask
    in parallel with a \l Do body. A \l Do body is initially on hold -
    it will continue after the barrier passed to the kicker will advance.
    This will happen as soon as the QProcess is started. Since than,
    QProcess runs in parallel with the \l Do body.

    \sa Do
*/
When::When(const BarrierKickerGetter &kicker, WorkflowPolicy policy)
    : d(new WhenPrivate{kicker, policy})
{}

/*!
    \fn template <typename Task, typename Adapter, typename Deleter, typename Signal> When::When(const QCustomTask<Task, Adapter, Deleter> &customTask, Signal signal, QtTaskTree::WorkflowPolicy policy = QtTaskTree::WorkflowPolicy::StopOnError)

    Creates a delaying element, taking \a customTask and its \c Task's
    \a signal to be run in parallel with a \l Do body. The \l Do body is
    delayed until the \c Task's \a signal is emitted. The \a customTask
    and the \l Do body will run in parallel using the \a policy.

    The code from the other When's constructor could be simplified to:

    \code
        const auto onSetup = [barrier](QProcess &process) {
            ... // Setup process program, arguments, environment, etc...
        };

        const Group recipe {
            When (QProcessTask(onSetup), &QProcess::started) >> Do {
                delayedTask1,
                ...
            }
        };
    \endcode

    \note The \c Task type of the passed \a customTask needs to be derived
          from QObject.

    \sa Do
*/

When::~When() = default;
When::When(const When &other) = default;
When::When(When &&other) noexcept = default;
When &When::operator=(const When &other) = default;

/*!
    Combines \a whenItem with \a doItem body and returns a \l Group
    ready to be used in task tree recipes.
*/
Group operator>>(const When &whenItem, const Do &doItem)
{
    const QStoredBarrier barrier;

    return {
        barrier,
        parallel,
        workflowPolicy(whenItem.d->m_workflowPolicy),
        whenItem.d->m_barrierKicker(barrier),
        Group {
            barrierAwaiterTask(barrier),
            doItem.children()
        }
    };
}

} // namespace QtTaskTree

QT_END_NAMESPACE
