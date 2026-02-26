// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include <QtTaskTree/qtasktreerunner.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

namespace QtTaskTree {

/*!
    \typealias QtTaskTree::TreeSetupHandler

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
    \typealias QtTaskTree::TreeDoneHandler

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

class QSingleTaskTreeRunnerPrivate
{
public:
    std::unique_ptr<QTaskTree> m_taskTree;
};

/*!
    \class QtTaskTree::QSingleTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule QtTaskTree
    \brief A single task tree execution controller.

    Manages single task tree execution.
    Use the start() method to execute a given recipe,
    resetting any possibly running task tree.
    It's guaranteed that at most one task tree is executing at any given time.

    \sa {Task Tree Runners}
*/

/*!
    Constructs a single task tree runner.
*/
QSingleTaskTreeRunner::QSingleTaskTreeRunner() : d_ptr(new QSingleTaskTreeRunnerPrivate) {}

/*!
    Destroys the single task tree runner. A possibly running
    task tree is deleted. No task tree's done handler is called.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/
QSingleTaskTreeRunner::~QSingleTaskTreeRunner() = default;

/*!
    Returns whether the single task tree runner is currently
    executing a task tree.
*/
bool QSingleTaskTreeRunner::isRunning() const { return bool(d_func()->m_taskTree); }

/*!
    Cancels the running task tree. Calls task tree' done
    handler with \l {QtTaskTree::} {DoneWith::Cancel}.
*/
void QSingleTaskTreeRunner::cancel()
{
    if (d_func()->m_taskTree)
        d_func()->m_taskTree->cancel();
}

/*!
    Resets the running task tree. No task tree's done handler is called.
*/
void QSingleTaskTreeRunner::reset()
{
    d_func()->m_taskTree.reset();
}

/*!
    \fn template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler> void QSingleTaskTreeRunner::start(const QtTaskTree::Group &recipe, SetupHandler &&setupHandler = {}, DoneHandler &&doneHandler = {}, QtTaskTree::CallDone callDone = QtTaskTree::CallDoneFlag::Always)

    Starts the \a recipe unconditionally, resetting any possibly
    running task tree.
    Calls \a setupHandler when new task tree is about to be started.
    Calls \a doneHandler when the task tree is finished.
    The \a doneHandler is called according to the passed \a callDone.
*/

void QSingleTaskTreeRunner::startImpl(const Group &recipe,
                                      const TreeSetupHandler &setupHandler,
                                      const TreeDoneHandler &doneHandler,
                                      CallDone callDone)
{
    Q_D(QSingleTaskTreeRunner);
    d->m_taskTree.reset(new QTaskTree(recipe));
    QObject::connect(d->m_taskTree.get(), &QTaskTree::done, d->m_taskTree.get(),
                     [d, doneHandler, callDone](DoneWith result) {
        QTaskTree *taskTree = d->m_taskTree.release();
        taskTree->deleteLater();
        if (doneHandler && shouldCallDone(callDone, result))
            doneHandler(*taskTree, result);
    });
    if (setupHandler)
        setupHandler(*d->m_taskTree);
    d->m_taskTree->start();
}

struct TreeData
{
    Group recipe;
    TreeSetupHandler setupHandler;
    TreeDoneHandler doneHandler;
    CallDone callDone;
};

class QSequentialTaskTreeRunnerPrivate
{
public:
    void startNext();

    QList<TreeData> m_treeDataQueue;
    QSingleTaskTreeRunner m_taskTreeRunner;
};

/*!
    \class QtTaskTree::QSequentialTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule QtTaskTree
    \brief A sequential task tree execution controller.

    Manages sequential task tree execution.
    Use the enqueue() method to schedule the execution of a given recipe.
    It's guaranteed that at most one task tree is executing at any given time.

    \sa {Task Tree Runners}
*/

void QSequentialTaskTreeRunnerPrivate::startNext()
{
    if (m_treeDataQueue.isEmpty())
        return;

    const auto data = m_treeDataQueue.takeFirst();
    const auto doneHandler = [this, doneHandler = data.doneHandler, callDone = data.callDone]
        (const QTaskTree &taskTree, QtTaskTree::DoneWith result) {
            if (doneHandler && shouldCallDone(callDone, result))
                doneHandler(taskTree, result);
            startNext();
        };
    m_taskTreeRunner.start(data.recipe, data.setupHandler, doneHandler);
}

/*!
    Constructs a sequential task tree runner.
*/
QSequentialTaskTreeRunner::QSequentialTaskTreeRunner() : d_ptr(new QSequentialTaskTreeRunnerPrivate) {}

/*!
    Destroys the sequential task tree runner. A possibly running
    task tree is deleted and enqueued tasks are removed.
    No task tree's done handler is called.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/
QSequentialTaskTreeRunner::~QSequentialTaskTreeRunner() = default;

/*!
    Returns whether the sequential task tree runner is currently
    executing a task tree.
*/
bool QSequentialTaskTreeRunner::isRunning() const
{
    Q_D(const QSequentialTaskTreeRunner);
    return !d->m_treeDataQueue.isEmpty() || d->m_taskTreeRunner.isRunning();
}

/*!
    Cancels the running task tree. Calls task tree' done
    handler with \l {QtTaskTree::} {DoneWith::Cancel}.
    All queued tasks are removed.
*/
void QSequentialTaskTreeRunner::cancel()
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_treeDataQueue.clear();
    d->m_taskTreeRunner.cancel();
}

/*!
    Resets the running task tree. No task tree's done handler is called.
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
    handler with \l {QtTaskTree::} {DoneWith::Cancel}.
    If there are any enqueued recipes, the dequeued recipe is started.
*/
void QSequentialTaskTreeRunner::cancelCurrent()
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_taskTreeRunner.cancel();
}

/*!
    Resets the running task tree. No task tree's done handler is called.
    If there are any enqueued recipes, the dequeued recipe is started.
*/
void QSequentialTaskTreeRunner::resetCurrent()
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_taskTreeRunner.reset();
    d->startNext();
}

/*!
    \fn template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler> void QSequentialTaskTreeRunner::enqueue(const QtTaskTree::Group &recipe, SetupHandler &&setupHandler = {}, DoneHandler &&doneHandler = {}, QtTaskTree::CallDone callDone = QtTaskTree::CallDoneFlag::Always)

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
                                            CallDone callDone)
{
    Q_D(QSequentialTaskTreeRunner);
    d->m_treeDataQueue.append({recipe, setupHandler, doneHandler, callDone});
    if (!d->m_taskTreeRunner.isRunning())
        d->startNext();
}

class QParallelTaskTreeRunnerPrivate
{
public:
    std::unordered_map<QTaskTree *, std::unique_ptr<QTaskTree>> m_taskTrees;
};

/*!
    \class QtTaskTree::QParallelTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule QtTaskTree
    \brief A parallel task tree execution controller.

    Manages parallel task tree execution.
    Use the start() method to execute a given recipe instantly,
    keeping other possibly running task trees in parallel.

    \sa {Task Tree Runners}
*/

/*!
    Constructs a parallel task tree runner.
*/
QParallelTaskTreeRunner::QParallelTaskTreeRunner() : d_ptr(new QParallelTaskTreeRunnerPrivate) {}

/*!
    Destroys the parallel task tree runner. All running
    task trees are deleted. No task trees' done handlers are called.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/
QParallelTaskTreeRunner::~QParallelTaskTreeRunner() = default;

/*!
    Returns whether the parallel task tree runner is currently
    executing at least one task tree.
*/
bool QParallelTaskTreeRunner::isRunning() const { return !d_func()->m_taskTrees.empty(); }

/*!
    Cancels all running task trees. Calls task trees' done
    handlers with \l {QtTaskTree::} {DoneWith::Cancel}.
    The order of task trees' cancellation is random.
*/
void QParallelTaskTreeRunner::cancel()
{
    Q_D(QParallelTaskTreeRunner);
    while (!d->m_taskTrees.empty())
        d->m_taskTrees.begin()->second->cancel();
}

/*!
    Resets all running task trees. No task trees' done handlers are called.
*/
void QParallelTaskTreeRunner::reset()
{
    d_func()->m_taskTrees.clear();
}

/*!
    \fn template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler> void QParallelTaskTreeRunner::start(const QtTaskTree::Group &recipe, SetupHandler &&setupHandler = {}, DoneHandler &&doneHandler = {}, QtTaskTree::CallDone callDone = QtTaskTree::CallDoneFlag::Always)

    Starts the \a recipe instantly and keeps other possibly running
    task trees in parallel.
    Calls \a setupHandler when new task tree is about to be started.
    Calls \a doneHandler when the task tree is finished.
    The \a doneHandler is called according to the passed \a callDone.
*/

void QParallelTaskTreeRunner::startImpl(const Group &recipe,
                                       const TreeSetupHandler &setupHandler,
                                       const TreeDoneHandler &doneHandler,
                                       CallDone callDone)
{
    Q_D(QParallelTaskTreeRunner);
    QTaskTree *taskTree = new QTaskTree(recipe);
    QObject::connect(taskTree, &QTaskTree::done, taskTree, [d, taskTree, doneHandler, callDone](DoneWith result) {
        const auto it = d->m_taskTrees.find(taskTree);
        it->second.release()->deleteLater();
        d->m_taskTrees.erase(it);
        if (doneHandler && shouldCallDone(callDone, result))
            doneHandler(*taskTree, result);
    });
    d->m_taskTrees.emplace(taskTree, taskTree);
    if (setupHandler)
        setupHandler(*taskTree);
    taskTree->start();
}

/*!
    \class QtTaskTree::QMappedTaskTreeRunner
    \inheaderfile qtasktreerunner.h
    \inmodule QtTaskTree
    \brief A mapped task tree execution controller with a given Key type.

    Manages mapped task tree execution using the \a Key type.
    Use the start() method to execute a recipe for a given key unconditionally,
    resetting a possibly running task tree with the same key,
    and keeping other possibly running task trees with different keys
    in parallel.

    \sa {Task Tree Runners}
*/

/*!
    \fn template <typename Key> QMappedTaskTreeRunner<Key>::QMappedTaskTreeRunner()

    Constructs a mapped task tree runner.
    The \c Key type is used for task tree mapping.
*/

/*!
    \fn template <typename Key> QMappedTaskTreeRunner<Key>::~QMappedTaskTreeRunner()

    Destroys the mapped task tree runner. All running
    task trees are deleted. No task trees' done handlers are called.

    \sa {QTaskTree::~QTaskTree()} {~QTaskTree()}
*/

/*!
    \fn template <typename Key> bool QMappedTaskTreeRunner<Key>::isRunning() const

    Returns whether the mapped task tree runner is currently
    executing at least one task tree.
*/

/*!
    \fn template <typename Key> void QMappedTaskTreeRunner<Key>::cancel()

    Cancels all running task trees. Calls task trees' done
    handlers with \l {QtTaskTree::} {DoneWith::Cancel}.
    The order of task trees' cancellation is random.
*/

/*!
    \fn template <typename Key> void QMappedTaskTreeRunner<Key>::reset()

    Resets all running task trees. No task trees' done handlers are called.
*/

/*!
    \fn template <typename Key> bool QMappedTaskTreeRunner<Key>::isKeyRunning(const Key &key) const

    Returns whether the mapped task tree runner is currently
    executing a task tree that was started with the \a key.
*/

/*!
    \fn template <typename Key> void QMappedTaskTreeRunner<Key>::cancelKey(const Key &key)

    Cancels a potentially running task tree that was started with the \a key.
    Calls task tree's done handler with \l {QtTaskTree::} {DoneWith::Cancel}.
*/

/*!
    \fn template <typename Key> void QMappedTaskTreeRunner<Key>::resetKey(const Key &key)

    Resets a potentially running task tree that was started with the \a key.
    No task tree's done handlers is called.
*/

/*!
    \fn template <typename Key> template <typename SetupHandler = TreeSetupHandler, typename DoneHandler = TreeDoneHandler> void QMappedTaskTreeRunner<Key>::start(const Key &key, const QtTaskTree::Group &recipe, SetupHandler &&setupHandler = {}, DoneHandler &&doneHandler = {}, QtTaskTree::CallDone callDone = QtTaskTree::CallDoneFlag::Always)

    Starts the \a recipe for a given \a key unconditionally,
    resetting any possibly running task tree with the same key,
    and keeping other possibly running task trees with different keys
    in parallel.
    Calls \a setupHandler when new task tree is about to be started.
    Calls \a doneHandler when the task tree is finished.
    The \a doneHandler is called according to the passed \a callDone.
*/

} // namespace QtTaskTree

QT_END_NAMESPACE
