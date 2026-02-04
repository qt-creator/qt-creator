// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTaskTree/qtasktreerunner.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

using namespace QtTaskTree;

class QAbstractTaskTreeRunnerPrivate : public QObjectPrivate
{
public:
    std::unique_ptr<QTaskTree> m_taskTree;
};

/*!
    \class QAbstractTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule TaskTree
    \brief An abstract base class for various task tree controllers.
    \reentrant

    The task tree runner manages the lifetime
    of the underlying QTaskTree used to execute the given recipe.

    The following table summarizes the differences between various
    QAbstractTaskTreeRunner subclasses:

    \table
    \header
        \li Class name
        \li Description
    \row
        \li QSingleTaskTreeRunner
        \li Manages single task tree execution.
            The QSingleTaskTreeRunner::start() method unconditionally starts
            the passed recipe, resetting any task tree that might be
            running. Only one task tree can be executing at a time.
    \row
        \li QSequentialTaskTreeRunner
        \li Manages sequential task tree executions.
            The QSequentialTaskTreeRunner::enqueue() method starts
            the passed recipe if the task tree runner is idle.
            Otherwise, the recipe is enqueued. When the current
            task finishes, the runner executes the dequeued recipe
            sequentially. Only one task tree can be executing at a time.
    \row
        \li QParallelTaskTreeRunner
        \li Manages parallel task tree executions.
            The QParallelTaskTreeRunner::start() method unconditionally starts
            the passed recipe and keeps any possibly
            running task trees in parallel.
    \row
        \li QMappedTaskTreeRunner
        \li Manages mapped task tree executions.
            The QMappedTaskTreeRunner::start() method unconditionally starts
            the passed recipe for a given key,
            resetting any possibly running task tree with the same key.
            Task trees with different keys are unaffected and continue
            their execution.
    \endtable
*/

/*!
    \typealias QAbstractTaskTreeRunner::TreeSetupHandler

    Type alias for std::function<void(QTaskTree &)>.

    The \c TreeSetupHandler is an optional argument of
    task tree runners' functions scheduling the task tree execution.
    The function is called when the task tree runner is about
    to execute it's recipe. The access to the QTaskTree is via
    the passed argument.

    The task tree runners accept also handlers in a shortened form
    of \c std::function<void(void)>.
*/

/*!
    \typealias QAbstractTaskTreeRunner::TreeDoneHandler

    Type alias for std::function<void(const QTaskTree &, QtTaskTree::DoneWith)>.

    The \c TreeDoneHandler is an optional argument of
    task tree runners' functions scheduling the task tree execution.
    The function is called when the task tree runner's recipe
    finished the execution. The access to the QTaskTree and recipe's
    result is via the passed arguments.

    The task tree runners accept also handlers in a shortened form of
    \c std::function<void(const QTaskTree &)>,
    \c std::function<void(QtTaskTree::DoneWith)>, or
    \c std::function<void(void)>.
*/

/*!
    Constructs an abstract task tree runner for the given \a parent.
*/
QAbstractTaskTreeRunner::QAbstractTaskTreeRunner(QObject *parent)
    : QObject(*new QAbstractTaskTreeRunnerPrivate, parent)
{}

QAbstractTaskTreeRunner::QAbstractTaskTreeRunner(QAbstractTaskTreeRunnerPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{}

/*!
    Destroys the abstract task tree runner. Any possibly running
    task tree is deleted and any scheduled task tree execution
    is skipped.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/
QAbstractTaskTreeRunner::~QAbstractTaskTreeRunner() = default;

/*!
    \fn bool QAbstractTaskTreeRunner::isRunning() const

    Returns whether the task tree runner is currently
    executing any task tree.
*/

/*!
    \fn void QAbstractTaskTreeRunner::cancel()

    Cancels all running task trees. Calls task trees' done
    handlers and emits done() signals with
    \l {QtTaskTree::} {DoneWith::Cancel}.
    Any scheduled and not started task tree executions are removed.
*/

/*!
    \fn void QAbstractTaskTreeRunner::reset()

    Resets all running task trees. No task tree's done
    handlers are called nor done() signals are emitted.
    Any scheduled and not started task tree executions are removed.
*/

/*!
    \fn void QAbstractTaskTreeRunner::aboutToStart(QTaskTree *taskTree)

    This signal is emitted whenever task tree runner is about to
    start the \a taskTree.

    \sa QTaskTree::started()
*/

/*!
    \fn void QAbstractTaskTreeRunner::done(QtTaskTree::DoneWith result, QTaskTree *taskTree)

    This signal is emitted whenever task tree runner finishes the execution
    of the \a taskTree with a \a result.

    \sa QTaskTree::done()
*/

class QSingleTaskTreeRunnerPrivate : public QAbstractTaskTreeRunnerPrivate
{
public:
    std::unique_ptr<QTaskTree> m_taskTree;
};

/*!
    \class QSingleTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule TaskTree
    \brief A single task tree execution controller.

    Manages single task tree execution.
    Use the start() method to execute a given recipe,
    resetting any possibly running task tree.
    It's guaranteed that at most one task tree is executing at any given time.
*/

/*!
    Constructs a single task tree runner for the given \a parent.
*/
QSingleTaskTreeRunner::QSingleTaskTreeRunner(QObject *parent)
    : QAbstractTaskTreeRunner(*new QSingleTaskTreeRunnerPrivate, parent)
{}

/*!
    Destroys the single task tree runner. A possibly running
    task tree is deleted. No task tree's done handler is called nor
    done() signal is emitted.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/
QSingleTaskTreeRunner::~QSingleTaskTreeRunner() = default;

/*!
    \reimp

    Returns whether the single task tree runner is currently
    executing a task tree.
*/
bool QSingleTaskTreeRunner::isRunning() const { return bool(d_func()->m_taskTree); }

/*!
    \reimp

    Cancels the running task tree. Calls task tree' done
    handler and emits done() signal with
    \l {QtTaskTree::} {DoneWith::Cancel}.
*/
void QSingleTaskTreeRunner::cancel()
{
    if (d_func()->m_taskTree)
        d_func()->m_taskTree->cancel();
}

/*!
    \reimp

    Resets the running task tree. No task tree's done
    handler is called nor done() signal is emitted.
*/
void QSingleTaskTreeRunner::reset()
{
    d_func()->m_taskTree.reset();
}

/*!
    \fn template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler> void QSingleTaskTreeRunner::start(const QtTaskTree::Group &recipe, SetupHandler &&setupHandler = {}, DoneHandler &&doneHandler = {}, QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)

    Starts the \a recipe unconditionally, resetting any possibly
    running task tree.
    Calls \a setupHandler when new task tree is about to be started.
    Calls \a doneHandler when the task tree is finished.
    The \a doneHandler is called according to the passed \a callDone.
*/

void QSingleTaskTreeRunner::startImpl(const Group &recipe,
                                      const TreeSetupHandler &setupHandler,
                                      const TreeDoneHandler &doneHandler,
                                      CallDoneFlags callDone)
{
    Q_D(QSingleTaskTreeRunner);
    d->m_taskTree.reset(new QTaskTree(recipe));
    connect(d->m_taskTree.get(), &QTaskTree::done, this, [this, doneHandler, callDone](DoneWith result) {
        Q_D(QSingleTaskTreeRunner);
        QTaskTree *taskTree = d->m_taskTree.release();
        taskTree->deleteLater();
        if (doneHandler && shouldCallDone(callDone, result))
            doneHandler(*taskTree, result);
        Q_EMIT done(result, taskTree);
    });
    if (setupHandler)
        setupHandler(*d->m_taskTree);
    Q_EMIT aboutToStart(d->m_taskTree.get());
    d->m_taskTree->start();
}

namespace QtTaskTree {

struct TreeData
{
    Group recipe;
    QAbstractTaskTreeRunner::TreeSetupHandler setupHandler;
    QAbstractTaskTreeRunner::TreeDoneHandler doneHandler;
    CallDoneFlags callDone;
};

} // namespace QtTaskTree

class QSequentialTaskTreeRunnerPrivate : public QAbstractTaskTreeRunnerPrivate
{
public:
    void startNext();

    QList<TreeData> m_treeDataQueue;
    QSingleTaskTreeRunner m_taskTreeRunner;
};

/*!
    \class QSequentialTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule TaskTree
    \brief A sequential task tree execution controller.

    Manages sequential task tree execution.
    Use the enqueue() method to schedule the execution of a given recipe.
    It's guaranteed that at most one task tree is executing at any given time.
*/

void QSequentialTaskTreeRunnerPrivate::startNext()
{
    if (m_treeDataQueue.isEmpty())
        return;

    const auto data = m_treeDataQueue.takeFirst();
    m_taskTreeRunner.start(data.recipe, data.setupHandler, data.doneHandler, data.callDone);
}

/*!
    Constructs a sequential task tree runner for the given \a parent.
*/
QSequentialTaskTreeRunner::QSequentialTaskTreeRunner(QObject *parent)
    : QAbstractTaskTreeRunner(*new QSequentialTaskTreeRunnerPrivate, parent)
{
    Q_D(QSequentialTaskTreeRunner);
    connect(&d->m_taskTreeRunner, &QSingleTaskTreeRunner::aboutToStart,
            this, &QSequentialTaskTreeRunner::aboutToStart);
    connect(&d->m_taskTreeRunner, &QSingleTaskTreeRunner::done,
            this, [this](DoneWith result, QTaskTree *taskTree) {
        Q_EMIT done(result, taskTree);
        d_func()->startNext();
    });
}

/*!
    Destroys the sequential task tree runner. A possibly running
    task tree is deleted and enqueued tasks are removed.
    No task tree's done handler is called nor done() signal is emitted.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/
QSequentialTaskTreeRunner::~QSequentialTaskTreeRunner() = default;

/*!
    \reimp

    Returns whether the sequential task tree runner is currently
    executing a task tree.
*/
bool QSequentialTaskTreeRunner::isRunning() const
{
    Q_D(const QSequentialTaskTreeRunner);
    return !d->m_treeDataQueue.isEmpty() || d->m_taskTreeRunner.isRunning();
}

/*!
    \reimp

    Cancels the running task tree. Calls task tree' done
    handler and emits done() signal with
    \l {QtTaskTree::} {DoneWith::Cancel}.
    All queued tasks are removed.
*/
void QSequentialTaskTreeRunner::cancel()
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_treeDataQueue.clear();
    d->m_taskTreeRunner.cancel();
}

/*!
    \reimp

    Resets the running task tree. No task tree's done
    handler is called nor done() signal is emitted.
    All queued tasks are removed.
*/
void QSequentialTaskTreeRunner::reset()
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_treeDataQueue.clear();
    d->m_taskTreeRunner.reset();
}

/*!
    Cancels the running task tree. Calls task tree' done
    handler and emits done() signal with
    \l {QtTaskTree::} {DoneWith::Cancel}.
    If there are any enqueued recipes, the dequeued recipe is started.
*/
void QSequentialTaskTreeRunner::cancelCurrent()
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_taskTreeRunner.cancel();
}

/*!
    Resets the running task tree. No task tree's done
    handler is called nor done() signal is emitted.
    If there are any enqueued recipes, the dequeued recipe is started.
*/
void QSequentialTaskTreeRunner::resetCurrent()
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_taskTreeRunner.reset();
    d->startNext();
}

/*!
    \fn template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler> void QSequentialTaskTreeRunner::enqueue(const QtTaskTree::Group &recipe, SetupHandler &&setupHandler = {}, DoneHandler &&doneHandler = {}, QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)

    Schedules the \a recipe execution. If no task tree is executing,
    the runner starts a new task tree synchronously, otherwise
    the \a recipe is enqueued. When the currently executing task tree
    finished, the runner starts a new task tree with a dequeued recipe.
    Calls \a setupHandler when new task tree is about to be started.
    Calls \a doneHandler when the task tree is finished.
    The \a doneHandler is called according to the passed \a callDone.
*/

void QSequentialTaskTreeRunner::enqueueImpl(const Group &recipe,
                                            const TreeSetupHandler &setupHandler,
                                            const TreeDoneHandler &doneHandler,
                                            CallDoneFlags callDone)
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_treeDataQueue.append({recipe, setupHandler, doneHandler, callDone});
    if (!d->m_taskTreeRunner.isRunning())
        d->startNext();
}

class QParallelTaskTreeRunnerPrivate : public QAbstractTaskTreeRunnerPrivate
{
public:
    std::unordered_map<QTaskTree *, std::unique_ptr<QTaskTree>> m_taskTrees;
};

/*!
    \class QParallelTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule TaskTree
    \brief A parallel task tree execution controller.

    Manages parallel task tree execution.
    Use the start() method to execute a given recipe instantly,
    keeping other possibly running task trees in parallel.
*/

/*!
    Constructs a parallel task tree runner for the given \a parent.
*/
QParallelTaskTreeRunner::QParallelTaskTreeRunner(QObject *parent)
    : QAbstractTaskTreeRunner(*new QParallelTaskTreeRunnerPrivate, parent)
{}

/*!
    Destroys the parallel task tree runner. All running
    task trees are deleted. No task trees' done handlers are called nor
    done() signals are emitted.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/
QParallelTaskTreeRunner::~QParallelTaskTreeRunner() = default;

/*!
    \reimp

    Returns whether the parallel task tree runner is currently
    executing at least one task tree.
*/
bool QParallelTaskTreeRunner::isRunning() const { return !d_func()->m_taskTrees.empty(); }

/*!
    \reimp

    Cancels all running task trees. Calls task trees' done
    handlers and emits done() signals with
    \l {QtTaskTree::} {DoneWith::Cancel}.
    The order of task trees' cancellation is random.
*/
void QParallelTaskTreeRunner::cancel()
{
    Q_D(QParallelTaskTreeRunner);
    while (!d->m_taskTrees.empty())
        d->m_taskTrees.begin()->second->cancel();
}

/*!
    \reimp

    Resets all running task trees. No task trees' done
    handlers are called nor done() signals are emitted.
*/
void QParallelTaskTreeRunner::reset()
{
    d_func()->m_taskTrees.clear();
}

/*!
    \fn template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler> void QParallelTaskTreeRunner::start(const QtTaskTree::Group &recipe, SetupHandler &&setupHandler = {}, DoneHandler &&doneHandler = {}, QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)

    Starts the \a recipe instantly and keeps other possibly running
    task trees in parallel.
    Calls \a setupHandler when new task tree is about to be started.
    Calls \a doneHandler when the task tree is finished.
    The \a doneHandler is called according to the passed \a callDone.
*/

void QParallelTaskTreeRunner::startImpl(const Group &recipe,
                                       const TreeSetupHandler &setupHandler,
                                       const TreeDoneHandler &doneHandler,
                                       CallDoneFlags callDone)
{
    Q_D(QParallelTaskTreeRunner);
    QTaskTree *taskTree = new QTaskTree(recipe);
    connect(taskTree, &QTaskTree::done, this, [this, taskTree, doneHandler, callDone](DoneWith result) {
        Q_D(QParallelTaskTreeRunner);
        const auto it = d->m_taskTrees.find(taskTree);
        it->second.release()->deleteLater();
        d->m_taskTrees.erase(it);
        if (doneHandler && shouldCallDone(callDone, result))
            doneHandler(*taskTree, result);
        Q_EMIT done(result, taskTree);
    });
    d->m_taskTrees.emplace(taskTree, taskTree);
    if (setupHandler)
        setupHandler(*taskTree);
    Q_EMIT aboutToStart(taskTree);
    taskTree->start();
}

/*!
    \class QMappedTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule TaskTree
    \brief A mapped task tree execution controller with a given Key type.

    Manages mapped task tree execution using \c Key type.
    Use the start() method to execute a recipe for a given key unconditionally,
    resetting a possibly running task tree with the same key,
    and keeping other possibly running task trees with different keys
    in parallel.
*/

/*!
    \fn template <typename Key> QMappedTaskTreeRunner<Key>::QMappedTaskTreeRunner(QObject *parent = nullptr)

    Constructs a mapped task tree runner for the given \a parent.
    The \c Key type is used for task tree mapping.
*/

/*!
    \fn template <typename Key> QMappedTaskTreeRunner<Key>::~QMappedTaskTreeRunner()

    Destroys the mapped task tree runner. All running
    task trees are deleted. No task trees' done handlers are called nor
    done() signals are emitted.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/

/*!
    \fn template <typename Key> bool QMappedTaskTreeRunner<Key>::isRunning() const
    \reimp

    Returns whether the mapped task tree runner is currently
    executing at least one task tree.
*/

/*!
    \fn template <typename Key> void QMappedTaskTreeRunner<Key>::cancel()
    \reimp

    Cancels all running task trees. Calls task trees' done
    handlers and emits done() signals with
    \l {QtTaskTree::} {DoneWith::Cancel}.
    The order of task trees' cancellation is random.
*/

/*!
    \fn template <typename Key> void QMappedTaskTreeRunner<Key>::reset()
    \reimp

    Resets all running task trees. No task trees' done
    handlers are called nor done() signals are emitted.
*/

/*!
    \fn template <typename Key> bool QMappedTaskTreeRunner<Key>::isKeyRunning(const Key &key) const

    Returns whether the mapped task tree runner is currently
    executing a task tree that was started with the \a key.
*/

/*!
    \fn template <typename Key> void QMappedTaskTreeRunner<Key>::cancelKey(const Key &key)

    Cancels a potentially running task tree that was started with the \a key.
    Calls task tree's done handler and emits done() signal with
    \l {QtTaskTree::} {DoneWith::Cancel}.
*/

/*!
    \fn template <typename Key> void QMappedTaskTreeRunner<Key>::resetKey(const Key &key)

    Resets a potentially running task tree that was started with the \a key.
    No task tree's done handlers is called nor done() signal is emitted.
*/

/*!
    \fn template <typename Key> template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler> void QMappedTaskTreeRunner<Key>::start(const Key &key, const QtTaskTree::Group &recipe, SetupHandler &&setupHandler = {}, DoneHandler &&doneHandler = {}, QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)

    Starts the \a recipe for a given \a key unconditionally,
    resetting any possibly running task tree with the same key,
    and keeping other possibly running task trees with different keys
    in parallel.
    Calls \a setupHandler when new task tree is about to be started.
    Calls \a doneHandler when the task tree is finished.
    The \a doneHandler is called according to the passed \a callDone.
*/

QT_END_NAMESPACE
