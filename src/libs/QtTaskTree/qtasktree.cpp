// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTaskTree/qtasktree.h>

#include <QtTaskTree/qbarriertask.h>
#include <QtTaskTree/qconditional.h>

#include <QtCore/private/qobject_p.h>

#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#if QT_CONFIG(future)
#  include <QtCore/QFutureWatcher>
#endif
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QMetaEnum>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QThread>
#include <QtCore/QTime>
#include <QtCore/QTimer>

using namespace std::chrono;

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \namespace QtTaskTree
    \inmodule TaskTree
    \brief The QtTaskTree namespace encloses helper classes and global
           functions of the TaskTree module.

    \sa {Qt TaskTree}
*/

/*!
    \module TaskTree
    \ingroup modules
    \title Qt TaskTree C++ Classes
    \brief Contains a general purpose TaskTree library.

    Use the TaskTree library to construct recipes, describing what
    asynchronous  tasks are to be executed, and use these recipes
    within QTaskTree to execute them.

    The recipes are declarative descriptions on what task types
    are to be created and executed, e.g.: QProcess, QNetworkReplyWrapper,
    or QThreadFunction<ReturnType>, or whether they should run in sequence
    or in parallel. Inside recipes you may define different continuation
    paths depending on whether the previous task finished with success
    or an error. It's also possible to nest tasks in
    \l {QtTaskTree::} {Group} elements, and each
    \l {QtTaskTree::} {Group} may run its tasks according to
    it's own execution mode or workflow policy.
    The recipes form the task tree structures.

    \section1 Asynchronous Tasks

    Asynchronous task is any task that can be started, and \e later it
    finishes with success or an error. \e Later means that after starting
    the task, the control goes back to the running event loop.
    We don’t block the caller thread until the task is finished.
    In order to use the task tree we need an event loop spinning.

    Examples of asynchronous Tasks:
    \list
        \li QTimer::singleShot()
        \li QProcess
        \li QNetworkAccessManager + QNetworkReply = QNetworkReplyWrapper
        \li QtConcurrent::run() + QFutureWatcher<Result>
            = QThreadFunction<Result>
    \endlist

    \section1 Recipe & Task Tree

    In order to memorize what the recipe and task tree is, let's use
    the analogy to the cartridge and player. When we write a recipe,
    it's like we would be constructing a cartridge, so we just prepare
    a detailed description on what the player should do later,
    when the cartridge is placed inside a player (task tree) and started.
    The recipe itself is just a declarative description for the task tree
    on what the task tree should do when the recipe is passed to the
    task tree and the task tree is started. The recipe itself doesn't do
    anything on its own without a task tree - it's like a cartridge
    without a player.

    Here is a short summary about recipe and task tree responsibilities.

    Recipe (cartridge) describes:
    \list
        \li what tasks are to be created dynamically by the running
            task tree (via QCustomTask)
        \li in which order
        \li what data structures are to be created dynamically by the running
            task tree (via \l {QtTaskTree::} {Storage})
        \li how to setup each task before start
        \li how to collect data when tasks are finished
        \li execution mode (tasks should run in sequence or in parallel)
        \li workflow policy
    \endlist

    Task tree (player):
    \list
        \li reads the recipe and creates tasks and data structures
            automatically
        \li manages the lifetime of created tasks and data structures
        \li executes continuations
        \li chooses different paths depending on results of finished tasks
            and workflow policies
        \li provides basic progress info
    \endlist

    \section1 Custom Tasks

    Since the recipe is a description for the task tree on what tasks it should
    create when the task tree is started, we can't create these tasks directly
    inside a recipe. Instead, we need a declarative way to tell the task tree
    to create and start these tasks for us at later point in time. For example,
    if we want the task tree to create and start QProcess, we describe it
    by placing QProcessTask element inside a recipe. The QProcessTask is an
    alias to the QCustomTask<QProcess>. Each task Type should provide
    its corresponding QCustomTask<Type> so that it may be used inside recipes.

    The following table shows some build-in custom tasks ready to be placed
    inside recipes:

    \table
    \header
        \li Custom Task (used in recipes)
        \li Task Class (created by the running task tree)
        \li Brief Description
    \row
        \li QProcessTask
        \li QProcess
        \li Starts process.
    \row
        \li QThreadFunctionTask<ReturnType>
        \li QThreadFunction<ReturnType>
        \li Starts asynchronous task, runs in separate thread.
    \row
        \li QTaskTreeTask
        \li QTaskTree
        \li Starts nested task tree.
    \row
        \li QNetworkReplyWrapperTask
        \li QNetworkReplyWrapper
        \li Starts network download.
    \row
        \li QTcpSocketWrapperTask
        \li QTcpSocketWrapper
        \li Starts a TCP connection.
    \endtable

    See QTaskInterface and \l {Task Adapters} for more information on
    how to adapt particular task to be used inside recipes.

    \section1 Example Recipe

    The QTaskTree has a top level Group element, a.k.a recipe, which may
    contain any number of tasks of various types, such as QProcessTask,
    QNetworkReplyWrapperTask, or QThreadFunctionTask<ReturnType>:

    \code
        const Group recipe {
            QProcessTask(...),
            QNetworkReplyWrapperTask(...),
            QThreadFunctionTask<int>(...)
        };

        QTaskTree *taskTree = new QTaskTree(recipe);
        connect(taskTree, &QTaskTree::done, ...);  // finish handler
        taskTree->start();
    \endcode

    The recipe above consist of a top level element of the Group type that
    contains tasks of the QProcessTask, QNetworkReplyWrapperTask,
    and QThreadFunctionTask<int> type. After taskTree->start() is called,
    the tasks are created and run in a chain, starting with QProcess.
    When the QProcess finishes successfully, the QNetworkReplyWrapper
    task is started. Finally, when the network task finishes successfully,
    the QThreadFunction<int> task is started.

    When the last running task finishes with success, the task tree is
    considered to have run successfully and the QTaskTree::done() signal
    is emitted with \l {QtTaskTree::} {DoneWith::Success}.
    When a task finishes with an error, the execution of the task tree
    is stopped and the remaining tasks are skipped. The task tree finishes
    with an error and sends the QTaskTree::done() signal with
    \l {QtTaskTree::} {DoneWith::Error}.

    \section1 Groups

    The parent of the \l {QtTaskTree::} {Group} sees it as a single task.
    Like other tasks, the group can be started and it can finish with
    success or an error. The \l {QtTaskTree::} {Group} elements
    can be nested to create a tree structure:

    \code
        const Group recipe {
            Group {
                parallel,
                QProcessTask(...),
                QThreadFunctionTask<int>(...)
            },
            QNetworkReplyWrapperTask(...)
        };
    \endcode

    The example above differs from the first example in that the top level
    element has a subgroup that contains the QProcessTask and
    QThreadFunctionTask<int>. The subgroup is a sibling element of the
    QNetworkReplyWrapperTask in the root. The subgroup contains an
    additional \e parallel element that instructs its Group to execute
    its tasks in parallel.

    So, when the QTaskTree starts the recipe above, the QProcess and
    QThreadFunction<int> start immediately and run in parallel.
    Since the root group doesn't contain a \e parallel element,
    its direct child tasks are run in sequence. Thus, the
    QNetworkReplyWrapper starts when the whole subgroup finishes. The group is
    considered as finished when all its tasks have finished.
    The order in which the tasks finish is not relevant.

    So, depending on which task lasts longer
    (QProcess or QThreadFunction<int>), the following scenarios can take place:

    \table
    \header
        \li Scenario 1
        \li Scenario 2
    \row
        \li Root Group starts
        \li Root Group starts
    \row
        \li Sub Group starts
        \li Sub Group starts
    \row
        \li QProcess starts
        \li QProcess starts
    \row
        \li QThreadFunction<int> starts
        \li QThreadFunction<int> starts
    \row
        \li ...
        \li ...
    \row
        \li \b {QProcess finishes}
        \li \b {QThreadFunction<int> finishes}
    \row
        \li ...
        \li ...
    \row
        \li \b {QThreadFunction<int> finishes}
        \li \b {QProcess finishes}
    \row
        \li Sub Group finishes
        \li Sub Group finishes
    \row
        \li QNetworkReplyWrapper starts
        \li QNetworkReplyWrapper starts
    \row
        \li ...
        \li ...
    \row
        \li QNetworkReplyWrapper finishes
        \li QNetworkReplyWrapper finishes
    \row
        \li Root Group finishes
        \li Root Group finishes
    \endtable

    The differences between the scenarios are marked with bold. Three dots mean
    that an unspecified amount of time passes between previous and next events
    (a task or tasks continue to run). No dots between events
    means that they occur synchronously.

    The presented scenarios assume that all tasks run successfully. If a task
    fails during execution, the task tree finishes with an error.
    In particular, when QProcess finishes with an error while
    QThreadFunction<int> is still being executed, the QThreadFunction<int>
    is automatically canceled, the subgroup finishes with an error,
    the QNetworkReplyWrapper is skipped, and the tree finishes with an error.

    \section1 Task Handlers

    Use Task handlers to set up a task for execution and to enable reading
    the output data from the task when it finishes with success or an error.

    \section2 Task's Start Handler

    When a task object is created and before it's started,
    the task tree invokes an optionally user-provided setup handler.
    The setup handler should always take a \e reference to the associated
    task class object:

    \code
        const auto onSetup = [](QProcess &process) {
            process.setProgram("sleep");
            process.setArguments({"3"});
        };
        const Group root {
            QProcessTask(onSetup)
        };
    \endcode

    You can modify the passed QProcess in the setup handler, so that the task
    tree can start the process according to your configuration.
    You should not call \c {process.start();} in the setup handler,
    as the task tree calls it when needed. The setup handler is optional.
    When used, it must be the first argument of the task's constructor.

    Optionally, the setup handler may return a
    \l {QtTaskTree::} {SetupResult}. The returned
    \l {QtTaskTree::} {SetupResult} influences the further
    start behavior of a given task. The possible values are:

    \table
    \header
        \li \l {QtTaskTree::} {SetupResult} Value
        \li Brief Description
    \row
        \li \l {QtTaskTree::SetupResult::} {Continue}
        \li The task will be started normally. This is the default behavior
            when the setup handler doesn't return
            \l {QtTaskTree::} {SetupResult}
            (that is, its return type is void).
    \row
        \li \l {QtTaskTree::SetupResult::} {StopWithSuccess}
        \li The task won't be started and it will report success to its parent.
    \row
        \li \l {QtTaskTree::SetupResult::} {StopWithError}
        \li The task won't be started and it will report an error to its parent.
    \endtable

    This is useful for running a task only when a condition is met and the data
    needed to evaluate this condition is not known until previously
    started tasks finish. In this way, the setup handler dynamically
    decides whether to start the corresponding task normally or skip it
    and report success or an error. For more information about inter-task
    data exchange, see \l {QtTaskTree::} {Storage}.

    \section2 Task's Done Handler

    When a running task finishes, the task tree invokes an optionally
    provided done handler. The handler should take a \c const \e reference
    to the associated task class object:

    \code
        const auto onSetup = [](QProcess &process) {
            process.setProgram("sleep");
            process.setArguments({"3"});
        };
        const auto onDone = [](const QProcess &process, DoneWith result) {
            if (result == DoneWith::Success)
                qDebug() << "Success" << process.cleanedStdOut();
            else
                qDebug() << "Failure" << process.cleanedStdErr();
        };
        const Group root {
            QProcessTask(onSetup, onDone)
        };
    \endcode

    The done handler may collect output data from QProcess, and store it
    for further processing or perform additional actions.

    \note If the task setup handler returns
          \l {QtTaskTree::SetupResult::} {StopWithSuccess} or
          \l {QtTaskTree::SetupResult::} {StopWithError},
          the done handler is not invoked.

    \section1 Group Handlers

    Similarly to task handlers, group handlers enable you to set up a group to
    execute and to apply more actions when the whole group finishes with
    success or an error.

    \section2 Group's Start Handler

    The task tree invokes the group start handler before it starts the child
    tasks. The group handler doesn't take any arguments:

    \code
        const auto onSetup = [] {
            qDebug() << "Entering the group";
        };
        const Group root {
            onGroupSetup(onSetup),
            QProcessTask(...)
        };
    \endcode

    The group setup handler is optional. To define a group setup handler,
    add an \l {QtTaskTree::} {onGroupSetup()} element to a group.
    The argument of \l {QtTaskTree::} {onGroupSetup()} is a user
    handler. If you add more than one
    \l {QtTaskTree::} {onGroupSetup()} element to a group,
    an assert is triggered at runtime that includes an error message.

    Like the task's start handler, the group start handler may return
    \l {QtTaskTree::} {SetupResult}.
    The returned \l {QtTaskTree::} {SetupResult} value affects
    the start behavior of the whole group. If you do not specify
    a group start handler, or its return type is void,
    the default group's action is \l {QtTaskTree::SetupResult::} {Continue},
    so that all tasks are started normally.
    Otherwise, when the start handler returns
    \l {QtTaskTree::SetupResult::} {StopWithSuccess} or
    \l {QtTaskTree::SetupResult::} {StopWithError},
    the tasks are not started (they are skipped) and the group itself
    reports success or an error, depending on the returned value, respectively.

    \code
        const Group root {
            onGroupSetup([] { qDebug() << "Root setup"; }),
            Group {
                onGroupSetup([] { qDebug() << "Group 1 setup"; return SetupResult::Continue; }),
                QProcessTask(...) // Process 1
            },
            Group {
                onGroupSetup([] { qDebug() << "Group 2 setup"; return SetupResult::StopWithSuccess; }),
                QProcessTask(...) // Process 2
            },
            Group {
                onGroupSetup([] { qDebug() << "Group 3 setup"; return SetupResult::StopWithError; }),
                QProcessTask(...) // Process 3
            },
            QProcessTask(...) // Process 4
        };
    \endcode

    In the above example, all subgroups of a root group define their
    setup handlers. The following scenario assumes that all started
    processes finish with success:

    \table
    \header
        \li Scenario
        \li Comment
    \row
        \li Root Group starts
        \li Doesn't return SetupResult, so its tasks are executed.
    \row
        \li Group 1 starts
        \li Returns Continue, so its tasks are executed.
    \row
        \li Process 1 starts
        \li
    \row
        \li ...
        \li ...
    \row
        \li Process 1 finishes (success)
        \li
    \row
        \li Group 1 finishes (success)
        \li
    \row
        \li Group 2 starts
        \li Returns StopWithSuccess, so Process 2 is skipped and Group 2
            reports success.
    \row
        \li Group 2 finishes (success)
        \li
    \row
        \li Group 3 starts
        \li Returns StopWithError, so Process 3 is skipped and Group 3
            reports an error.
    \row
        \li Group 3 finishes (error)
        \li
    \row
        \li Root Group finishes (error)
        \li Group 3, which is a direct child of the root group,
            finished with an error, so the root group stops executing,
            skips Process 4, which has not started yet, and reports an error.
    \endtable

    \section2 Groups's Done Handler

    A \l {QtTaskTree::} {Group}'s done handler is executed after
    the successful or failed execution of its tasks.
    The final value reported by the group depends on its \l {Workflow Policy}.
    The handler can apply other necessary actions.
    The done handler is defined inside the \l {QtTaskTree::} {onGroupDone()}
    element of a group. It may take the optional
    \l {QtTaskTree::} {DoneWith} argument, indicating
    the successful or failed execution:

    \code
        const Group root {
            onGroupSetup([] { qDebug() << "Root setup"; }),
            QProcessTask(...),
            onGroupDone([](DoneWith result) {
                if (result == DoneWith::Success)
                    qDebug() << "Root finished with success";
                else
                    qDebug() << "Root finished with an error";
            })
        };
    \endcode

    The group done handler is optional. If you add more than one
    \l {QtTaskTree::} {onGroupDone()} to a group,
    an assert is triggered at runtime that includes an error message.

    \note Even if the group setup handler returns
    \l {QtTaskTree::SetupResult::} {StopWithSuccess} or
    \l {QtTaskTree::SetupResult::} {StopWithError},
    the group's done handler is invoked. This behavior differs from that
    of task done handler and might change in the future.

    \section1 Other Group Elements

    A group can contain other elements that describe the processing flow,
    such as the execution mode or workflow policy. It can also contain
    storage elements that are responsible for collecting and sharing
    custom common data gathered during group execution.

    \section2 Execution Mode

    The execution mode element in a Group specifies how the direct
    child tasks of the Group are started. The most common execution modes are
    \l {QtTaskTree::} {sequential} and
    \l {QtTaskTree::} {parallel}. It's also possible to specify
    the limit of tasks running in parallel by using the
    \l {QtTaskTree::} {ParallelLimit} element.

    In all execution modes, a group starts tasks in the oder
    in which they appear.

    If a child of a group is also a group, the child group runs its tasks
    according to its own execution mode.

    \section2 Workflow Policy

    The workflow policy element in a \l {QtTaskTree::} {Group}
    specifies how the group should behave when any of its \e direct
    child's tasks finish. For a detailed description of possible policies,
    refer to \l {QtTaskTree::} {WorkflowPolicy}.

    If a child of a group is also a group, the child group runs its tasks
    according to its own workflow policy.

    \section2 Storage

    Use the \l {QtTaskTree::} {Storage} element to exchange
    information between tasks. Especially, in the sequential execution mode,
    when a task needs data from another, already finished task,
    before it can start. For example, a task tree that copies data
    by reading it from a source and writing it to a destination
    might look as follows:

    \code
        static QByteArray load(const QString &fileName) { ... }
        static void save(const QString &fileName, const QByteArray &array) { ... }

        static Group copyRecipe(const QString &source, const QString &destination)
        {
            struct CopyStorage { // [1] custom inter-task struct
                QByteArray content; // [2] custom inter-task data
            };

            // [3] instance of custom inter-task struct manageable by task tree
            const Storage<CopyStorage> storage;

            const auto onLoaderSetup = [source](QThreadFunction<QByteArray> &async) {
                async.setThreadFunctionData(&load, source);
            };
            // [4] runtime: task tree activates the instance from [7] before invoking handler
            const auto onLoaderDone = [storage](const QThreadFunction<QByteArray> &async) {
                storage->content = async.result(); // [5] loader stores the result in storage
            };

            // [4] runtime: task tree activates the instance from [7] before invoking handler
            const auto onSaverSetup = [storage, destination](QThreadFunction<void> &async) {
                const QByteArray content = storage->content; // [6] saver takes data from storage
                async.setThreadFunctionData(&save, destination, content);
            };
            const auto onSaverDone = [](const QThreadFunction<void> &async) {
                qDebug() << "Save done successfully";
            };

            const Group root {
                // [7] runtime: task tree creates an instance of CopyStorage when root is entered
                storage,
                QThreadFunctionTask<QByteArray>(onLoaderSetup, onLoaderDone, CallDone::OnSuccess),
                QThreadFunctionTask<void>(onSaverSetup, onSaverDone, CallDone::OnSuccess)
            };
            return root;
        }

        ...

        const QString source = ...;
        const QString destination = ...;
        QTaskTree taskTree(copyRecipe(source, destination));
        connect(&taskTree, &QTaskTree::done, &taskTree, [](DoneWith result) {
            if (result == DoneWith::Success)
                qDebug() << "The copying finished successfully.";
        });
        tasktree.start();
    \endcode

    In the example above, the inter-task data consists of a QByteArray content
    variable [2] enclosed in a \c CopyStorage custom struct [1]. If the loader
    finishes successfully, it stores the data in a \c CopyStorage::content
    variable [5]. The saver then uses the variable to configure the
    saving task [6].

    To enable a task tree to manage the \c CopyStorage struct, an instance of
    \l {QtTaskTree::} {Storage}<\c {CopyStorage}> is created [3].
    If a copy of this object is inserted as the group's child item [7],
    an instance of the \c CopyStorage struct is created dynamically
    when the task tree enters this group. When the task tree leaves this group,
    the existing instance of the \c CopyStorage struct is destructed
    as it's no longer needed.

    If several task trees holding a copy of the common
    \l {QtTaskTree::} {Storage}<\c {CopyStorage}> instance run
    simultaneously (including the case when the task trees are run
    in different threads), each task tree contains its own copy of
    the \c CopyStorage struct.

    You can access \c CopyStorage from any handler in the group with
    a storage object. This includes all handlers of all descendant tasks
    of the group with a storage object. To access the custom struct
    in a handler, pass the copy of the
    \l {QtTaskTree::} {Storage}<\c {CopyStorage}> object to the handler
    (for example, in a lambda capture) [4].

    When the task tree invokes a handler in a subtree containing
    the storage [7], the task tree activates its own \c CopyStorage
    instance inside the \l {QtTaskTree::} {Storage}<\c {CopyStorage}>
    object. Therefore, the \c CopyStorage struct may be accessed
    only from within the handler body. To access the currently active
    \c CopyStorage from within
    \l {QtTaskTree::} {Storage}<\c {CopyStorage}>, use the
    \l {QtTaskTree::} {Storage::operator->()},
    \l {QtTaskTree::} {Storage::operator*()}, or
    \l {QtTaskTree::} {Storage::activeStorage()}
       method.

    The following list summarizes how to employ a Storage object into the task
    tree:
    \list 1
        \li Define the custom structure \c MyStorage with custom data [1], [2]
        \li Create an instance of the \l {QtTaskTree::}
            {Storage}<\c {MyStorage}> storage [3]
        \li Pass the \l {QtTaskTree::} {Storage}<\c {MyStorage}>
            instance to handlers [4]
        \li Access the \c MyStorage instance in handlers [5], [6]
        \li Insert the \l {QtTaskTree::} {Storage}<\c {MyStorage}>
            instance into a group [7]
    \endlist

    \section1 QTaskTree class

    QTaskTree executes the tree structure of asynchronous tasks according
    to the recipe described by the \l {QtTaskTree::} {Group}
    root element.

    As QTaskTree is also an asynchronous task, it can be a part of another
    QTaskTree. To place a nested QTaskTree inside another QTaskTree,
    insert the QTaskTreeTask element into another
    \l {QtTaskTree::} {Group} element.

    QTaskTree reports progress of completed tasks when running.
    The progress value is increased when a task finishes or is skipped
    or canceled. When QTaskTree is finished and the QTaskTree::done()
    signal is emitted, the current value of the progress equals the
    maximum progress value. Maximum progress equals the total number
    of asynchronous tasks in a tree. A nested QTaskTree is counted
    as a single task, and its child tasks are not counted in the
    top level tree. Groups themselves are not counted as tasks,
    but their tasks are counted. \l QSyncTask tasks are not asynchronous,
    so they are not counted as tasks.

    To set additional initial data for the running tree, modify the storage
    instances in a tree when it creates them by installing a storage setup
    handler:

    \code
        Storage<CopyStorage> storage;
        const Group root = ...; // storage placed inside root's group and inside handlers
        QTaskTree taskTree(root);
        auto initStorage = [](CopyStorage &storage) {
            storage.content = "initial content";
        };
        taskTree.onStorageSetup(storage, initStorage);
        taskTree.start();
    \endcode

    When the running task tree creates a \c CopyStorage instance,
    and before any handler inside a tree is called, the task tree calls
    the \c initStorage handler, to enable setting up initial data
    of the storage, unique to this particular run of taskTree.

    Similarly, to collect some additional result data from the running tree,
    read it from storage instances in the tree when they are about to be
    destroyed. To do this, install a storage done handler:

    \code
        Storage<CopyStorage> storage;
        const Group root = ...; // storage placed inside root's group and inside handlers
        QTaskTree taskTree(root);
        auto collectStorage = [](const CopyStorage &storage) {
            qDebug() << "final content" << storage.content;
        };
        taskTree.onStorageDone(storage, collectStorage);
        taskTree.start();
    \endcode

    When the running task tree is about to destroy a \c CopyStorage instance,
    the task tree calls the collectStorage handler, to enable reading
    the final data from the storage, unique to this particular run of taskTree.

    \section1 Task Adapters

    Allowing new Task types to be a part of recipes is quite easy.
    It's enough to define a new task alias to the QCustomTask template,
    passing your Task type as a first template argument, like:

    \code
        class Worker : public QObject
        {
        public:
            void start() { ... }

        signals:
            void done(bool result);
        };

        using WorkerTask = QCustomTask<Worker>;
    \endcode

    This is going to work, if the following conditions are met:
    \list 1
        \li Your task is derived from QObject.
        \li Your task has public start() method that starts the task.
        \li Your task emits done(bool) or
            done(\l {QtTaskTree::} {DoneResult}) signal
            when it's finished.
    \endlist

    If your task doesn't meet these conditions, you may still adapt your
    task to work with the TaskTree framework, by providing a second
    template argument with the custom adapter.
    Let's say, we want to adapt QTimer to work with TaskTree.
    The Adapter could look like:

    \code
        class TimerAdapter
        {
        public:
            void operator()(QTimer *task, QTaskInterface *iface) {
                task->setSingleShot(true);
                QObject::connect(task, &QTimer::timeout, iface, [iface] {
                    iface->reportDone(DoneResult::Success);
                });
                task->start();
            }
        };

        using TimerTask = QCustomTask<QTimer, TimerAdapter>;
    \endcode

    Now you may start using the TimerTask in your recipes, like:

    \code
        const auto onSetup = [](QTimer &task) {
            task.setInterval(2000);
        };

        const auto onDone = [](const QTimer &task) {
            qDebug() << "Timer triggered after" << task.interval() << "ms.";
        };

        const Group recipe {
            TimerTask(onSetup, onDone)
        };
    \endcode

    \note The class implementing the running task should have
    a default constructor, and objects of this class should be
    freely destructible. It should be allowed to destroy a running task,
    preferably without waiting for the running task to finish
    (that is, safe non-blocking destructor of a running task).
    To achieve a non-blocking destruction of a task that has
    a blocking destructor, consider using the optional \c Deleter
    template parameter of the QCustomTask (the third template argument).
*/

namespace QtTaskTree {

/*!
    \typealias QtTaskTree::GroupItems

    Type alias for QList<GroupItem>.
*/

/*!
    \class QtTaskTree::GroupItem
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief GroupItem represents the basic element that may be a part of any
           \l {QtTaskTree::} {Group}.
    \reentrant

    GroupItem is a basic element that may be a part of any
    \l {QtTaskTree::} {Group}. It encapsulates the functionality
    provided by any GroupItem's subclass.
    It is a value type and it is safe to copy the GroupItem instance,
    even when it is originally created via the subclass' constructor.

    There are four main kinds of GroupItem:
    \table
    \header
        \li \l {QtTaskTree::} {GroupItem} Kind
        \li Brief Description
    \row
        \li \l QCustomTask
        \li Defines asynchronous task type and task's start, done,
            and error handlers. Aliased with a unique task name, such as,
            QThreadFunctionTask<ResultType> or QNetworkReplyWrapperTask.
            Asynchronous tasks are the main reason for using a task tree.
    \row
        \li \l {QtTaskTree::} {Group}
        \li A container for other group items. Since the group is of the
            GroupItem type, it's possible to nest it inside another group.
            The group is seen by its parent as a single asynchronous task.
    \row
        \li GroupItem containing \l {QtTaskTree::} {Storage}
        \li Enables the child tasks of a group to exchange data.
            When GroupItem containing \l {QtTaskTree::} {Storage}
            is placed inside a group, the task tree instantiates
            the storage's data object just before the group is entered,
            and destroys it just after the group is left.
    \row
        \li Other group control items
        \li The \l {QtTaskTree::} {ExecutionMode} or items returned by
            \l {QtTaskTree::} {workflowPolicy()}
            influence the group's behavior. The items returned by
            \l {QtTaskTree::} {onGroupSetup()} or
            \l {QtTaskTree::} {onGroupDone()} define custom
            handlers called when the group starts or ends execution.
    \endtable
*/

/*!
    \class QtTaskTree::Group
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief Group represents the basic element for composing declarative
           recipes describing how to execute and handle a nested tree
           of asynchronous tasks.
    \reentrant

    Group is a container for other group items. It encloses child tasks
    into one unit, which is seen by the group's parent as a single,
    asynchronous task. Since Group is of the GroupItem type,
    it may also be a child of Group.

    Insert child tasks into the group by using aliased custom task names,
    such as, QThreadFunctionTask<ResultType> or QNetworkReplyWrapperTask:

    \code
        const Group group {
            QNetworkReplyWrapperTask(...),
            QThreadFunctionTask<int>(...)
        };
    \endcode

    You can customize the group's behavior by inserting the
    \l {QtTaskTree::}{ExecutionMode} or
    \l {QtTaskTree::}{WorkflowPolicy} items:

    \code
        const Group group {
            parallel,
            continueOnError,
            QNetworkReplyWrapperTask(...),
            QNetworkReplyWrapperTask(...)
        };
    \endcode

    The group may contain nested groups:

    \code
        const Group group {
            finishAllAndSuccess,
            QNetworkReplyWrapperTask(...),
            Group {
                QNetworkReplyWrapperTask(...),
                Group {
                    parallel,
                    QNetworkReplyWrapperTask(...),
                    QNetworkReplyWrapperTask(...),
                }
                QThreadFunctionTask<QString>(...)
            }
        };
    \endcode

    The group may dynamically instantiate a custom storage structure,
    which may be used for inter-task data exchange:

    \code
        struct MyCustomStruct { QByteArray data; };

        Storage<MyCustomStruct> storage;

        const auto onFirstSetup = [](QNetworkReplyWrapper &task) { ... };
        const auto onFirstDone = [storage](const QNetworkReplyWrapper &task) {
            // storage-> gives a pointer to MyCustomStruct instance,
            // created dynamically by the running task tree.
            storage->data = task.reply()->readAll();
        };
        const auto onSecondSetup = [storage](QThreadFunction<QImage> &task) {
            // storage-> gives a pointer to MyCustomStruct. Since the group is sequential,
            // the stored MyCustomStruct was already updated inside the onFirstDone handler.
            const QByteArray storedData = storage->data;
        };

        const Group group {
            // When the group is entered by a running task tree, it creates MyCustomStruct
            // instance dynamically. It is later accessible from all handlers via
            // the *storage or storage-> operators.
            sequential,
            storage,
            QNetworkReplyWrapperTask(onFirstSetup, onFirstDone, CallDone::OnSuccess),
            QThreadFunctionTask<QImage>(onSecondSetup)
        };
    \endcode
*/

/*!
    \enum QtTaskTree::WorkflowPolicy

    This enum describes the possible behavior of the Group element when any
    group's child task finishes its execution. It's also used when the
    running Group is canceled.

    \value StopOnError
        Default. Corresponds to the stopOnError global element.
        If any child task finishes with an error, the group stops and
        finishes with an error. If all child tasks finished with success,
        the group finishes with success. If a group is empty,
        it finishes with success.
    \value ContinueOnError
        Corresponds to the continueOnError global element.
        Similar to stopOnError, but in case any child finishes with an error,
        the execution continues until all tasks finish, and the group
        reports an error afterwards, even when some other tasks in the
        group finished with success. If all child tasks finish successfully,
        the group finishes with success. If a group is empty,
        it finishes with success.
    \value StopOnSuccess
        Corresponds to the stopOnSuccess global element.
        If any child task finishes with success, the group stops and
        finishes with success. If all child tasks finished with an error,
        the group finishes with an error. If a group is empty,
        it finishes with an error.
    \value ContinueOnSuccess
        Corresponds to the continueOnSuccess global element.
        Similar to stopOnSuccess, but in case any child finishes successfully,
        the execution continues until all tasks finish, and the group
        reports success afterwards, even when some other tasks in the group
        finished with an error. If all child tasks finish with an error,
        the group finishes with an error. If a group is empty,
        it finishes with an error.
    \value StopOnSuccessOrError
        Corresponds to the stopOnSuccessOrError global element.
        The group starts as many tasks as it can. When any task finishes,
        the group stops and reports the task's result.
        Useful only in parallel mode.
        In sequential mode, only the first task is started, and when finished,
        the group finishes too, so the other tasks are always skipped.
        If a group is empty, it finishes with an error.
    \value FinishAllAndSuccess
        Corresponds to the finishAllAndSuccess global element.
        The group executes all tasks and ignores their return results.
        When all tasks finished, the group finishes with success.
        If a group is empty, it finishes with success.
    \value FinishAllAndError
        Corresponds to the finishAllAndError global element.
        The group executes all tasks and ignores their return results.
        When all tasks finished, the group finishes with an error.
        If a group is empty, it finishes with an error.

    Whenever a child task's result causes the Group to stop, that is,
    in case of StopOnError, StopOnSuccess, or StopOnSuccessOrError policies,
    the Group cancels the other running child tasks
    (if any - for example in parallel mode), and skips executing tasks
    it has not started yet (for example, in the sequential mode -
    those, that are placed after the failed task). Both canceling and
    skipping child tasks may happen when ParallelLimit is used.

    The table below summarizes the differences between various
    workflow policies:

    \table
    \header
        \li \l WorkflowPolicy
        \li Executes all child tasks
        \li Result
        \li Result when the group is empty
    \row
        \li StopOnError
        \li Stops when any child task finished with an error and
            reports an error
        \li An error when at least one child task failed, success otherwise
        \li Success
    \row
        \li ContinueOnError
        \li Yes
        \li An error when at least one child task failed, success otherwise
        \li Success
    \row
        \li StopOnSuccess
        \li Stops when any child task finished with success and reports success
        \li Success when at least one child task succeeded, an error otherwise
        \li An error
    \row
        \li ContinueOnSuccess
        \li Yes
        \li Success when at least one child task succeeded, an error otherwise
        \li An error
    \row
        \li StopOnSuccessOrError
        \li Stops when any child task finished and reports child task's result
        \li Success or an error, depending on the finished child task's result
        \li An error
    \row
        \li FinishAllAndSuccess
        \li Yes
        \li Success
        \li Success
    \row
        \li FinishAllAndError
        \li Yes
        \li An error
        \li An error
    \endtable

    If a child of a group is also a group, the child group runs its tasks
    according to its own workflow policy. When a parent group stops the
    running child group because of parent group's workflow policy, that is,
    when the StopOnError, StopOnSuccess, or StopOnSuccessOrError
    policy was used for the parent, the child group's result is reported
    according to the \b Result column and to the
    \b {child group's workflow policy} row in the table above.
*/

/*!
    \variable QtTaskTree::nullItem

    A convenient global group's element indicating a no-op item.

    This is useful in conditional expressions to indicate the absence
    of an optional element:

    \code
        const ExecutableItem task = ...;
        const std::optional<ExecutableItem> optionalTask = ...;

        Group group {
            task,
            optionalTask ? *optionalTask : nullItem
        };
    \endcode
*/

/*!
    \variable QtTaskTree::successItem

    A convenient global executable element containing an empty, successful,
    synchronous task.

    This is useful in if-statements to indicate that a branch
    ends with success:

    \code
        const ExecutableItem conditionalTask = ...;

        const Group group {
            stopOnSuccess,
            If (conditionalTask) >> Then {
                ...
            } >> Else {
                successItem
            },
            nextTask
        };
    \endcode

    In the above example, if the \c conditionalTask finishes with an error,
    the \l Else branch is chosen, which finishes immediately with success.
    This causes the \c nextTask to be skipped (because of the \l stopOnSuccess
    workflow policy of the \c group) and the \c group finishes with success.

    \sa errorItem
*/

/*!
    \variable QtTaskTree::errorItem

    A convenient global executable element containing an empty,
    erroneous, synchronous task.

    This is useful in if-statements to indicate that a branch ends
    with an error:

    \code
        const ExecutableItem conditionalTask = ...;

        Group group {
            stopOnError,
            If (conditionalTask) >> Then {
                ...
            } >> Else {
                errorItem
            },
            nextTask
        };
    \endcode

    In the above example, if the \c conditionalTask finishes with an error,
    the \l Else branch is chosen, which finishes immediately with an error.
    This causes the \c nextTask to be skipped (because of the \l stopOnError
    workflow policy of the \c group) and the \c group finishes with an error.

    \sa successItem
*/

/*!
    \variable QtTaskTree::sequential
    A convenient global group's element describing the sequential
    execution mode.

    This is the default execution mode of the Group element.

    When a Group has no execution mode, it runs in the sequential mode.
    All the direct child tasks of a group are started in a chain,
    so that when one task finishes, the next one starts.
    This enables you to pass the results from the previous task
    as input to the next task before it starts. This mode guarantees that
    the next task is started only after the previous task finishes.

    \sa parallel, ParallelLimit
*/

/*!
    \variable QtTaskTree::parallel
    A convenient global group's element describing the parallel execution mode.

    All the direct child tasks of a group are started after the group
    is started, without waiting for the previous child tasks to finish.
    In this mode, all child tasks run simultaneously.

    \sa sequential, ParallelLimit
*/

/*!
    \variable QtTaskTree::parallelIdealThreadCountLimit
    A convenient global group's element describing the parallel
    execution mode with a limited number of tasks running simultanously.
    The limit is equal to the ideal number of threads excluding the
    calling thread.

    This is a shortcut to:
    \code
        ParallelLimit(qMax(QThread::idealThreadCount() - 1, 1))
    \endcode

    \sa parallel, ParallelLimit
*/

/*!
    \variable QtTaskTree::stopOnError
    A convenient global group's element describing the
    StopOnError workflow policy.

    This is the default workflow policy of the Group element.
*/

/*!
    \variable QtTaskTree::continueOnError
    A convenient global group's element describing the ContinueOnError
    workflow policy.
*/

/*!
    \variable QtTaskTree::stopOnSuccess
    A convenient global group's element describing the StopOnSuccess
    workflow policy.
*/

/*!
    \variable QtTaskTree::continueOnSuccess
    A convenient global group's element describing the ContinueOnSuccess
    workflow policy.
*/

/*!
    \variable QtTaskTree::stopOnSuccessOrError
    A convenient global group's element describing the StopOnSuccessOrError
    workflow policy.
*/

/*!
    \variable QtTaskTree::finishAllAndSuccess
    A convenient global group's element describing the FinishAllAndSuccess
    workflow policy.
*/

/*!
    \variable QtTaskTree::finishAllAndError
    A convenient global group's element describing the FinishAllAndError
    workflow policy.
*/

/*!
    \enum QtTaskTree::SetupResult

    This enum is optionally returned from the group's or task's setup
    handler function. It instructs the running task tree on how to proceed
    after the setup handler's execution finished.
    \value Continue
           Default. The group's or task's execution continues normally.
           When a group's or task's setup handler returns void,
           it's assumed that it returned Continue.
    \value StopWithSuccess
           The group's or task's execution stops immediately with success.
           When returned from the group's setup handler, all child tasks
           are skipped, and the group's onGroupDone() handler is invoked
           with DoneWith::Success. The group reports success to its parent.
           The group's workflow policy is ignored.
           When returned from the task's setup handler, the task isn't started,
           its done handler isn't invoked, and the task reports success
           to its parent.
    \value StopWithError
           The group's or task's execution stops immediately with an error.
           When returned from the group's setup handler, all child tasks
           are skipped, and the group's onGroupDone() handler is invoked
           with DoneWith::Error. The group reports an error to its parent.
           The group's workflow policy is ignored.
           When returned from the task's setup handler, the task isn't started,
           its error handler isn't invoked, and the task reports an error
           to its parent.
*/

/*!
    \enum QtTaskTree::DoneResult

    This enum is optionally returned from the group's or task's done handler
    function. When the done handler doesn't return any value, that is,
    its return type is \c void, its final return value is automatically
    deduced by the running task tree and reported to its parent group.

    When the done handler returns the DoneResult, you can tweak
    the final return value inside the handler.

    When the DoneResult is returned by the group's done handler,
    the group's workflow policy is ignored.

    This enum is also used inside the QTaskInterface::done() signal and
    it indicates whether the task finished with success or an error.

    \value Success
           The group's or task's execution ends with success.
    \value Error
           The group's or task's execution ends with an error.
*/

/*!
    \enum QtTaskTree::DoneWith

    This enum is an optional argument for the group's or task's done handler.
    It indicates whether the group or task finished with success or an error,
    or it was canceled.

    It is also used as an argument inside the QTaskTree::done() signal,
    indicating the final result of the QTaskTree execution.

    \value Success
           The group's or task's execution ended with success.
    \value Error
           The group's or task's execution ended with an error.
    \value Cancel
           The group's or task's execution was canceled. This happens when
           the user calls QTaskTree::cancel() for the running task tree or
           when the group's workflow policy results in canceling some of
           its running children. Tweaking the done handler's final result
           by returning DoneResult from the handler is no-op
           when the group's or task's execution was canceled.
*/

/*!
    \enum QtTaskTree::CallDone

    This enum is an optional argument for the \l onGroupDone()
    element or custom task's constructor. It instructs the task tree
    on when the group's or task's done handler should be invoked.

    \value Never
           The done handler is never invoked.
    \value OnSuccess
           The done handler is invoked only after successful execution,
           that is, when DoneWith::Success.
    \value OnError
           The done handler is invoked only after failed execution,
           that is, when DoneWith::Error.
    \value OnCancel
           The done handler is invoked only after canceled execution,
           that is, when DoneWith::Cancel.
    \value Always
           The done handler is always invoked.
*/

/*!
    \typealias QtTaskTree::GroupItem::GroupSetupHandler

    Type alias for \c std::function<SetupResult()>.

    The GroupSetupHandler is an argument of the onGroupSetup() element.
    Any function with the above signature, when passed as a group setup handler,
    will be called by the running task tree when the group execution starts.

    The return value of the handler instructs the running group on
    how to proceed after the handler's invocation is finished.
    The default return value of SetupResult::Continue instructs the group
    to continue running, that is, to start executing its child tasks.
    The return value of SetupResult::StopWithSuccess or
    SetupResult::StopWithError instructs the group to skip the child tasks'
    execution and finish immediately with success or an error, respectively.

    When the return type is either SetupResult::StopWithSuccess
    or SetupResult::StopWithError, the group's done handler (if provided)
    is called synchronously immediately afterwards.

    \note Even if the group setup handler returns StopWithSuccess
    or StopWithError, the group's done handler is invoked.
    This behavior differs from that of task done handler and might change
    in the future.

    The onGroupSetup() element accepts also functions in the shortened form of
    \c std::function<void()>, that is, the return value is \c void.
    In this case, it's assumed that the return value is SetupResult::Continue.

    \sa onGroupSetup(), GroupDoneHandler, QCustomTask::TaskSetupHandler
*/

/*!
    \typealias QtTaskTree::GroupItem::GroupDoneHandler

    Type alias for \c std::function<DoneResult(DoneWith)> or DoneResult.

    The GroupDoneHandler is an argument of the onGroupDone() element.
    Any function with the above signature, when passed as a group done handler,
    will be called by the running task tree when the group execution ends.

    The DoneWith argument is optional and your done handler may omit it.
    When provided, it holds the info about the final result of a group
    that will be reported to its parent.

    The returned DoneResult value is optional and your handler may return
    \c void instead. In this case, the final result of the group will
    be equal to the value indicated by the DoneWith argument.
    When the handler returns the DoneResult value, the group's final
    result may be tweaked inside the done handler's body by the returned value.

    For a GroupDoneHandler of the DoneResult type, no additional
    handling is executed, and the group finishes unconditionally
    with the passed value of DoneResult, ignoring the group's workflow policy.

    \sa onGroupDone(), GroupSetupHandler, QCustomTask::TaskDoneHandler
*/

/*!
    \class QtTaskTree::Storage
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief A class template for custom data exchange in the running task tree.
    \reentrant

    The Storage class template is responsible for dynamically creating
    and destructing objects of the custom \c StorageStruct type.
    The creation and destruction are managed by the running task tree.
    If a Storage object is placed inside a \l {QtTaskTree::} {Group}
    element, the running task tree creates the \c StorageStruct object
    when the group is started and before the group's setup handler is called.
    Later, whenever any handler inside this group is called,
    the task tree activates the previously created instance of the
    \c StorageStruct object. This includes all tasks' and groups'
    setup and done handlers inside the group where the Storage object
    was placed, also within the nested groups. When a copy of the Storage
    object is passed to the handler via the lambda capture, the handler
    may access the instance activated by the running task tree via the
    \l {QtTaskTree::} {Storage::operator->()},
    \l {QtTaskTree::} {Storage::operator*()}, or activeStorage() method.
    If two handlers capture the same Storage object,
    one of them may store a custom data there, and the other may
    read it afterwards. When the group is finished, the previously created
    instance of the \c StorageStruct object is destroyed after the
    group's done handler is called.

    An example of data exchange between tasks:

    \code
        const Storage<QString> storage;

        const auto onFirstDone = [storage](const FirstWorker &task) {
            // Assings QString, taken from the first task result, to the active QString instance
            // of the Storage object.
            *storage = task.getResultAsString();
        };

        const auto onSecondSetup = [storage](SecondWorker &task) {
            // Reads QString from the active QString instance of the Storage object and use it to
            // configure the second task before start.
            task.configureWithString(*storage);
        };

        const Group root {
            // The running task tree creates QString instance when root in entered
            storage,
            // The done handler of the first task stores the QString in the storage
            FirstWorkerTask(..., onFirstDone),
            // The setup handler of the second task reads the QString from the storage
            SecondWorkerTask(onSecondSetup, ...)
        };
    \endcode

    Since the root group executes its tasks sequentially, the
    \c onFirstDone handler is always called before the
    \c onSecondSetup handler. This means that the QString data,
    read from the \c storage inside the \c onSecondSetup handler's body,
    has already been set by the \c onFirstDone handler.
    You can always rely on it in \l {QtTaskTree::} {sequential}
    execution mode.

    The Storage internals are shared between all of its copies.
    That is why the copies of the Storage object inside the handlers'
    lambda captures still refer to the same Storage instance.
    You may place multiple Storage objects inside one
    \l {QtTaskTree::} {Group} element, provided that they do not
    include copies of the same Storage object. Otherwise, an assert
    is triggered at runtime that includes an error message.
    However, you can place copies of the same Storage object in different
    \l {QtTaskTree::} {Group} elements of the same recipe.
    In this case, the running task tree will create multiple instances
    of the \c StorageStruct objects (one for each copy)
    and storage shadowing will take place. Storage shadowing works in a
    similar way to C++ variable shadowing inside the nested blocks of code:

    \code
        Storage<QString> storage;

        const Group root {
            storage,                            // Top copy, 1st instance of StorageStruct
            onGroupSetup([storage] { ... }),    // Top copy is active
            Group {
                storage,                        // Nested copy, 2nd instance of StorageStruct,
                                                // shadows Top copy
                onGroupSetup([storage] { ... }) // Nested copy is active
            },
            Group {
                onGroupSetup([storage] { ... }) // Top copy is active
            }
        };
    \endcode

    The Storage objects may also be used for passing the initial data
    to the executed task tree, and for reading the final data out of
    the task tree before it finishes. To do this, use
    \l {QTaskTree::} {onStorageSetup()} or
    \l {QTaskTree::} {onStorageDone()}, respectively.

    \note If you use an unreachable Storage object inside the handler,
          because you forgot to place the storage in the recipe,
          or placed it, but not in any handler's ancestor group,
          you may expect a crash, preceded by the following message:
          \e {The referenced storage is not reachable in the running tree.
          A nullptr will be returned which might lead to a crash in the
          calling code. It is possible that no storage was added to the tree,
          or the storage is not reachable from where it is referenced.}
*/

/*!
    \fn template <typename StorageStruct> Storage<StorageStruct>::Storage<StorageStruct>()

    Creates a storage for the given \c StorageStruct type.
    Whenever the running QTaskTree enters a \l Group where the Storage is placed,
    it creates the \c StorageStruct using default constructor.

    \note All copies of \c this object are considered to be the same
          Storage instance.
*/

/*!
    \fn template <typename StorageStruct> template <typename ...Args> Storage<StorageStruct>::Storage<StorageStruct>(const Args &...args)

    Creates a storage for the given \c StorageStruct type. The passed
    \a args are stored when creating a storage, and are used later by
    the running QTaskTree to construct the \c StorageStruct
    with the stored \a args.

    \note All copies of \c this object are considered to be the same
          Storage instance.
*/

/*!
    \fn template <typename StorageStruct> StorageStruct &Storage<StorageStruct>::operator*() const noexcept

    Returns a \e reference to the active \c StorageStruct object,
    created by the running task tree. Use this function only from inside
    the handler body of any GroupItem element placed in the recipe,
    otherwise you may expect a crash. Make sure that Storage is placed
    in any group ancestor of the handler's group item.

    \note The returned reference is valid as long as the group that
          created this instance is still running.

    \sa activeStorage(), operator->()
*/

/*!
    \fn template <typename StorageStruct> StorageStruct *Storage<StorageStruct>::operator->() const noexcept

    Returns a \e pointer to the active \c StorageStruct object,
    created by the running task tree. Use this function only from inside
    the handler body of any GroupItem element placed in the recipe,
    otherwise you may expect a crash. Make sure that Storage is placed
    in any group ancestor of the handler's group item.

    \note The returned pointer is valid as long as the group that created
          this instance is still running.

    \sa activeStorage(), operator*()
*/

/*!
    \fn template <typename StorageStruct> StorageStruct *Storage<StorageStruct>::activeStorage() const

    Returns a \e pointer to the active \c StorageStruct object,
    created by the running task tree. Use this function only from inside
    the handler body of any GroupItem element placed in the recipe,
    otherwise you may expect a crash. Make sure that Storage is placed
    in any group ancestor of the handler's group item.

    \note The returned pointer is valid as long as the group that
          created this instance is still running.

    \sa operator->(), operator*()
*/

/*!
    \fn template <typename StorageStruct> GroupItem::GroupItem(const Storage<StorageStruct> &storage)

    Constructs a GroupItem element holding the \a storage object.

    When the \l {QtTaskTree::} {Group} element containing
    \e this GroupItem is entered by the running task tree, an instance
    of the \c StorageStruct is created dynamically.

    When that group is about to be left after its execution,
    the previously instantiated \c StorageStruct is deleted.

    The dynamically created instance of \c StorageStruct is accessible
    from inside any handler body of the parent \l {QtTaskTree::} {Group}
    element, including nested groups and its tasks, via the
    \l {QtTaskTree::} {Storage::operator->()},
    \l {QtTaskTree::} {Storage::operator*()},
    or Storage::activeStorage() method.

    \sa {QtTaskTree::} {Storage}
*/

/*!
    \fn template <typename Handler> GroupItem onGroupSetup(Handler &&handler)

    Constructs a group's element holding the group setup handler.
    The \a handler is invoked whenever the group starts.

    The passed \a handler is either of the \c std::function<SetupResult()>
    or the \c std::function<void()> type. For more information on a
    possible handler type, refer to \l {GroupItem::GroupSetupHandler}.

    When the \a handler is invoked, none of the group's child tasks
    are running yet.

    If a group contains the Storage elements, the \a handler is invoked
    after the storages are constructed, so that the \a handler may already
    perform some initial modifications to the active storages.

    \sa GroupItem::GroupSetupHandler, onGroupDone()
*/

/*!
    \fn template <typename Handler> GroupItem onGroupDone(Handler &&handler, CallDoneFlags callDone = CallDone::Always)

    Constructs a group's element holding the group done handler.
    By default, the \a handler is invoked whenever the group finishes.
    Pass a non-default value for the \a callDone argument when you want
    the handler to be called only on a successful, failed,
    or canceled execution. Depending on the group's workflow policy,
    this handler may also be called when the running group is canceled
    (e.g. when stopOnError element was used).

    The passed \a handler is of the \c std::function<DoneResult(DoneWith)> type.
    Optionally, each of the return DoneResult type or the argument
    DoneWith type may be omitted (that is, its return type may be \c void).
    For more information on a possible handler type,
    refer to \l {GroupItem::GroupDoneHandler}.

    When the \a handler is invoked, all of the group's child tasks
    are already finished.

    If a group contains the Storage elements, the \a handler is invoked
    before the storages are destructed, so that the \a handler may still
    perform a last read of the active storages' data.

    \sa GroupItem::GroupDoneHandler, onGroupSetup()
*/

#define QT_TASKTREE_STRING(cond) qDebug("SOFT ASSERT: \"%s\" in %s: %s", cond,  __FILE__, QT_STRINGIFY(__LINE__))
#define QT_TASKTREE_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QT_TASKTREE_STRING(#cond); action; } do {} while (0)
#define QT_TASKTREE_CHECK(cond) if (cond) {} else { QT_TASKTREE_STRING(#cond); } do {} while (0)

class Guard
{
    Q_DISABLE_COPY(Guard)
public:
    Guard() = default;
    ~Guard() { QT_TASKTREE_CHECK(m_lockCount == 0); }
    bool isLocked() const { return m_lockCount; }
private:
    int m_lockCount = 0;
    friend class GuardLocker;
};

class GuardLocker
{
    Q_DISABLE_COPY(GuardLocker)
public:
    GuardLocker(Guard &guard) : m_guard(guard) { ++m_guard.m_lockCount; }
    ~GuardLocker() { --m_guard.m_lockCount; }
private:
    Guard &m_guard;
};

/*!
    \class QtTaskTree::ExecutionMode
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief The group element describing execution mode.
    \reentrant

    The \l{Execution Mode}{execution mode} element in a Group specifies
    how the direct child tasks of the Group are started.

    For convenience, you can use the \l sequential, \l parallel,
    \l parallelIdealThreadCountLimit, or \l ParallelLimit
    elements instead.

    In all execution modes, a group starts tasks in the order in
    which they appear.

    If a child of a group is also a group, the child group runs its tasks
    according to its own execution mode.

    \sa sequential, parallel, parallelIdealThreadCountLimit, ParallelLimit
*/

ExecutionMode::ExecutionMode(int limit)
    : GroupItem({{}, limit})
{}

/*!
    \class QtTaskTree::ParallelLimit
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief The parallel execution mode with a custom limit.
    \reentrant

    The \a limit defines the maximum number of direct child tasks
    running in parallel:

    \list
        \li When \a limit equals to 0, there is no limit, and all direct
        child tasks are started together, in the oder in which they appear
        in a group. This means the fully parallel execution, and the
        \l parallel element may be used instead.

        \li When \a limit equals to 1, it means that only one child task
        may run at the time. This means the sequential execution,
        and the \l sequential element may be used instead.
        In this case, child tasks run in chain, so the next child task
        starts after the previous child task has finished.

        \li When other positive number is passed as \a limit, the group's
        child tasks run in parallel, but with a limited number of tasks
        running simultanously. The \e limit defines the maximum number
        of tasks running in parallel in a group.
        When the group is started, the first batch of tasks is started
        (the number of tasks in a batch equals to the passed \a limit, at most),
        while the others are kept waiting. When any running task finishes,
        the group starts the next remaining one, so that the \a limit
        of simultaneously running tasks inside a group isn't exceeded.
        This repeats on every child task's finish until all child tasks
        are started. This enables you to limit the maximum number of tasks
        that run simultaneously, for example if running too many
        processes might block the machine for a long time.
    \endlist

    \sa ExecutionMode
*/

/*!
    Constructs a parallel execution mode with a given \a limit.

    \sa ExecutionMode
*/
ParallelLimit::ParallelLimit(int limit)
    : ExecutionMode(limit)
{}

/*!
    Constructs a group's \l {Workflow Policy} {workflow policy} element
    for a given \a policy.

    For convenience, global elements may be used instead.

    \sa stopOnError, continueOnError, stopOnSuccess, continueOnSuccess,
        stopOnSuccessOrError, finishAllAndSuccess, finishAllAndError,
        WorkflowPolicy
*/
GroupItem workflowPolicy(WorkflowPolicy policy)
{
    struct WorkflowPolicyItem : GroupItem {
         WorkflowPolicyItem(WorkflowPolicy policy) : GroupItem({{}, {}, policy}) {}
    };
    return WorkflowPolicyItem(policy);
}

const ExecutionMode sequential = ParallelLimit(1);
const ExecutionMode parallel = ParallelLimit(0);
const ExecutionMode parallelIdealThreadCountLimit = ParallelLimit(qMax(QThread::idealThreadCount() - 1, 1));

const GroupItem stopOnError = workflowPolicy(WorkflowPolicy::StopOnError);
const GroupItem continueOnError = workflowPolicy(WorkflowPolicy::ContinueOnError);
const GroupItem stopOnSuccess = workflowPolicy(WorkflowPolicy::StopOnSuccess);
const GroupItem continueOnSuccess = workflowPolicy(WorkflowPolicy::ContinueOnSuccess);
const GroupItem stopOnSuccessOrError = workflowPolicy(WorkflowPolicy::StopOnSuccessOrError);
const GroupItem finishAllAndSuccess = workflowPolicy(WorkflowPolicy::FinishAllAndSuccess);
const GroupItem finishAllAndError = workflowPolicy(WorkflowPolicy::FinishAllAndError);

// Keep below the above in order to avoid static initialization fiasco.
const GroupItem nullItem = Group {};
const ExecutableItem successItem = Group { finishAllAndSuccess };
const ExecutableItem errorItem = Group { finishAllAndError };

class ForPrivate : public QSharedData
{
public:
    ForPrivate(const Iterator &iterator) : m_iterator(iterator) {}
    Iterator m_iterator;
};

For::~For() = default;
For::For(const For &other) = default;
For::For(For &&other) noexcept = default;
For &For::operator=(const For &other) = default;

/*!
    \class QtTaskTree::For
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief A for loop element.
    \reentrant

    A for element, holding an iterator, enabling loop constructs like:

    \code
        For (RepeatIterator(5)) >> Do {
            task1,
            task2
        }
    \endcode

    \sa Iterator, Do
*/

/*!
    Constructs the For loop element, holding an \a iterator. Use the
    \l {QtTaskTree::operator>>(const QtTaskTree::For &forItem, const QtTaskTree::Do &doItem)}
    {stream insertion operator} with \l Do element to place the whole construct
    in a task tree recipe.
*/
For::For(const Iterator &iterator) : d(new ForPrivate{iterator}) {}

Iterator For::iterator() const
{
    return d->m_iterator;
}

class DoPrivate : public QSharedData
{
public:
    DoPrivate(const GroupItem &children) : m_children(children) {}
    GroupItem m_children;
};

Do::~Do() = default;
Do::Do(const Do &other) = default;
Do::Do(Do &&other) noexcept = default;
Do &Do::operator=(const Do &other) = default;

/*!
    \class QtTaskTree::Do
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief A body element used with For and When constructs.
    \reentrant

    A body element, holding a list of tasks to be executed on each
    \l For iteration or after \l When's barrier was advanced.

    \code
        For (RepeatIterator(5)) >> Do {
            task1,
            task2
        };
    \endcode

    In case of For loop, the Do's body will be executed multiple times.
    If onGroupSetup() or onGroupDone() handlers are a part of passed
    children to Do's constructor, they will be executed just once -
    before the whole loop starts, and after the whole loop is finished:

    \code
        const RepeatIterator iterator(3);

        const auto onSetup = [] { qDebug() << "Setup"; };

        const auto onSync = [iterator] { qDebug() << "Current iteration:" << iterator->iteration(); };

        const auto onDone = [] { qDebug() << "Done"; };

        const Group recipe = For (iterator) >> Do {
            onGroupSetup(onSetup),
            QSyncTask(onSync),
            onDone(onDone)
        };
    \endcode

    The above recipe, when executed, will output:

    \code
        Setup
        Current iteration: 0
        Current iteration: 1
        Current iteration: 2
        Done
    \endcode

    If the intention is to invoke group handlers on each iteration,
    enclose the Do body with an extra \l Group, like:

    \code
        const Group recipe = For (iterator) >> Do {
            Group {
                onGroupSetup(onSetup),
                QSyncTask(onSync),
                onDone(onDone)
            }
        };
    \endcode

    The latter recipe, when executed, will output:

    \code
        Setup
        Current iteration: 0
        Done
        Setup
        Current iteration: 1
        Done
        Setup
        Current iteration: 2
        Done
    \endcode

    The similar happens when the \l Storage passed as a direct child
    of the body. The storage structure is instantiated just once,
    before the whole loop starts, and is destroyed just after the
    whole loop is finished. If the intention is to have separate
    instance of the \l Storage for each iteration, enclose the Do body with
    an extra \l Group.

    Be careful with placing \l parallel element in the Do's body
    of the \l For loop, as all iterations will start in parallel.
    This might not be desired, especially in case of ForeverIterator.

    \sa For, When
*/

/*!
    Constructs a Do body with \a children to be executed on each
    \l For's iteration or after \l When's barrier was advanced.
*/
Do::Do(const GroupItems &children) : d(new DoPrivate{children}) {}


/*!
    \overload

    Constructs a Do body with \a children passed as initializer list.
*/
Do::Do(std::initializer_list<GroupItem> children) : d(new DoPrivate{children}) {}

GroupItem Do::children() const
{
    return d->m_children;
}

/*!
    Combines \a forItem with \a doItem body and returns a \l Group
    ready to be used in task tree recipes.
*/
Group operator>>(const For &forItem, const Do &doItem)
{
    return {forItem.iterator(), doItem.children()};
}

/*!
    \class QtTaskTree::Forever
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief Infinite loop of subtasks.
    \reentrant

    The tasks passed to the constructor will repeat until some of them causes
    the body's workflow policy to stop executing. By default, the body
    will be repeated until some task finishes with an error.

    Refer to \l Do documentation for more information about placing
    group handlers and \l {QtTaskTree::} {Storage} as a direct
    children of the Forever body.

    \sa For, ForeverIterator
*/

/*!
    Constructs an infinite loop that executes \a children.

    The following construction:

    \code
        const Group recipe = Forever {
            task1,
            task2
        };
    \endcode

    is an equivalent to:

    \code
        const Group recipe = For (ForeverIterator()) >> Do {
            task1,
            task2
        };
    \endcode
*/
Forever::Forever(const GroupItems &children)
{
    addChildren({ For (ForeverIterator()) >> Do { children } } );
}

/*!
    \overload

    Constructs an infinite loop that executes \a children passed
    as initializer list.
*/
Forever::Forever(std::initializer_list<GroupItem> children)
{
    addChildren({ For (ForeverIterator()) >> Do { children } } );
}

/*!
    Creates QTimeoutTask with \a timeout duration, after which the task
    finishes with \a result.

    \sa QTimeoutTask
*/
ExecutableItem timeoutTask(const std::chrono::milliseconds &timeout, DoneResult result)
{
    return QTimeoutTask([timeout](std::chrono::milliseconds &t) { t = timeout; }, result);
}

// Please note the thread_local keyword below guarantees a separate instance per thread.
// The s_activeTaskTrees is currently used internally only and is not exposed in the public API.
// It serves for withLog() implementation now. Add a note here when a new usage is introduced.
static thread_local QList<QTaskTree *> s_activeTaskTrees = {};

static QTaskTree *activeTaskTree()
{
    QT_TASKTREE_ASSERT(s_activeTaskTrees.size(), return nullptr);
    return s_activeTaskTrees.back();
}

DoneResult toDoneResult(bool success)
{
    return success ? DoneResult::Success : DoneResult::Error;
}

static SetupResult toSetupResult(bool success)
{
    return success ? SetupResult::StopWithSuccess : SetupResult::StopWithError;
}

static DoneResult toDoneResult(DoneWith doneWith)
{
    return doneWith == DoneWith::Success ? DoneResult::Success : DoneResult::Error;
}

static DoneWith toDoneWith(DoneResult result)
{
    return result == DoneResult::Success ? DoneWith::Success : DoneWith::Error;
}

template <typename T>
class LocalThreadStorage
{
public:
    T &data() {
        QMutexLocker lock(&m_threadDataMutex);
        return m_threadDataMap.try_emplace(QThread::currentThread()).first->second;
    }
private:
    QMutex m_threadDataMutex = {};
    // Use std::map on purpose, so that it doesn't invalidate references on modifications.
    // Don't optimize it by using std::unordered_map.
    std::map<QThread *, T> m_threadDataMap = {};
};

class IteratorThreadData
{
    Q_DISABLE_COPY_MOVE(IteratorThreadData)

public:
    IteratorThreadData() = default;
    void pushIteration(int index)
    {
        m_activeIteratorStack.push_back(index);
    }
    void popIteration()
    {
        QT_TASKTREE_ASSERT(m_activeIteratorStack.size(), return);
        m_activeIteratorStack.pop_back();
    }
    int iteration() const
    {
        QT_TASKTREE_ASSERT(m_activeIteratorStack.size(), qWarning(
            "The referenced iterator is not reachable in the running tree. "
            "A -1 will be returned which might lead to a crash in the calling code. "
            "It is possible that no iterator was added to the tree, "
            "or the iterator is not reachable from where it is referenced."); return -1);
        return m_activeIteratorStack.last();
    }

private:
    QList<int> m_activeIteratorStack;
};

class IteratorData
{
public:
    IteratorThreadData &threadData() { return m_threadData.data(); }

    const std::optional<int> m_loopCount = {};
    const Iterator::ValueGetter m_valueGetter = {};
    const Iterator::Condition m_condition = {};
    LocalThreadStorage<IteratorThreadData> m_threadData = {};
};

/*!
    \class QtTaskTree::Iterator
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief Base class to be used as an iterator inside For element.
    \reentrant

    \sa For, ForeverIterator, RepeatIterator, UntilIterator, ListIterator
*/

Iterator::Iterator()
    : m_iteratorData(new IteratorData)
{}

Iterator::Iterator(int count, const ValueGetter &valueGetter)
    : m_iteratorData(new IteratorData{count, valueGetter})
{}

Iterator::Iterator(const Condition &condition)
    : m_iteratorData(new IteratorData{{}, {}, condition})
{}

/*!
    Returns the iteration index of the currently executing handler inside
    a \l Do body of the \l For (\l Iterator) >> \l Do {} construct.
    Use this function only from inside the handler body of any
    GroupItem element placed in the \l Do body of the recipe,
    otherwise you may expect a crash. Make sure that \l Iterator
    is passed to the \l For element.

    Example usage:

    \code
        const QList<std::chrono::seconds> timeouts = { 5s, 1s, 3s };
        const ListIterator iterator(timeouts);

        const auto onSetup = [iterator](std::chrono::milliseconds &timeout) {
            timeout = *iterator;
            qDebug() << "Starting" << iterator.iteration() << "iteration with timeout"
                     << *iterator << "seconds.";
        };
        const auto onDone = [iterator] {
            qDebug() << "Finished" << iterator.iteration() << "iteration with timeout"
                     << *iterator << "seconds.";
        };

        const Group sequentialRecipe = For(iterator) >> Do {
            QTimeoutTask(onSetup, onDone)
        };

        const Group parallelRecipe = For(iterator) >> Do {
            parallel,
            QTimeoutTask(onSetup, onDone)
        };
    \endcode

    The output when executing \c sequentialRecipe will be:
    \code
        Starting 0 iteration with timeout 5s seconds.
        Finished 0 iteration with timeout 5s seconds.
        Starting 1 iteration with timeout 1s seconds.
        Finished 1 iteration with timeout 1s seconds.
        Starting 2 iteration with timeout 3s seconds.
        Finished 2 iteration with timeout 3s seconds.
    \endcode

    In sequential mode the order of iteration indices in done handlers
    is guaranteed to be preserved.

    The output when executing \c parallelRecipe will be:
    \code
        Starting 0 iteration with timeout 5s seconds.
        Starting 1 iteration with timeout 1s seconds.
        Starting 2 iteration with timeout 3s seconds.
        Finished 1 iteration with timeout 1s seconds.
        Finished 2 iteration with timeout 3s seconds.
        Finished 0 iteration with timeout 5s seconds.
    \endcode

    In parallel mode the order of iteration indices in done handlers
    isn't guaranteed to be preserved, and depends on the order
    of finished tasks. The returned iteration index inside the parallel
    \l Do body's done handler matches the index of the original iteration
    for the corresponding setup handler, so the order of iteration indices
    in subsequent done handlers may not be ascending.
*/
int Iterator::iteration() const
{
    return m_iteratorData->threadData().iteration();
}

const void *Iterator::valuePtr() const
{
    return m_iteratorData->m_valueGetter(iteration());
}

/*!
    \class QtTaskTree::ForeverIterator
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief Infinite iterator to be used inside For element.
    \reentrant

    \sa Iterator, RepeatIterator, UntilIterator, ListIterator
*/

/*!
    Constructs never ending iterator for the For (ForeverIterator()) >>
    Do {} construct.

    \note Be very careful if your Do body specifies parallel mode -
    in this case the QTaskTree may instantiate infinite number of tasks.

    Example usage:

    \code
        static const int maxNumber = 10;
        const int luckyNumber = QRandomGenerator().global()->bounded(maxNumber);

        qDebug() << "Today's lucky number is:" << luckyNumber;
        qDebug() << "Let's start the drawing...";

        const auto onNumberCheck = [luckyNumber] {
            const int drawnNumber = QRandomGenerator().global()->bounded(maxNumber);
            qDebug() << "You have drawn:" << drawnNumber;
            bool won = drawnNumber == luckyNumber;
            if (won)
                qDebug() << "You have won! Congratulations!";
            return won;
        };

        const Group recipe = For (ForeverIterator()) >> Do {
            stopOnSuccess,
            timeoutTask(1s),
            QSyncTask(onNumberCheck)
        };
    \endcode

    The possible output when the \c recipe is started by the QTaskTree:
    \code
        Today's lucky number is: 8
        Let's start the drawing...
        You have drawn: 2
        You have drawn: 2
        You have drawn: 6
        You have drawn: 7
        You have drawn: 9
        You have drawn: 8
        You have won! Congratulations!
    \endcode
*/
ForeverIterator::ForeverIterator() : Iterator() {}

/*!
    \class QtTaskTree::RepeatIterator
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief The repetitive iterator to be used inside For element.
    \reentrant

    \sa Iterator, ForeverIterator, UntilIterator, ListIterator
*/

/*!
    Constructs the repetitive iterator for the For (RepeatIterator(count)) >>
    Do {} construct. The iterator will repeat \a count times.

    Example usage:

    \code
        static const int maxTries = 5;
        static const int maxNumber = 10;
        const int luckyNumber = QRandomGenerator().global()->bounded(maxNumber);

        qDebug() << "Today's lucky number is:" << luckyNumber;
        qDebug() << "You have" << maxTries << "lottery tickets.";
        qDebug() << "Let's start the drawing...";

        const auto onNumberCheck = [luckyNumber] {
            const int drawnNumber = QRandomGenerator().global()->bounded(maxNumber);
            qDebug() << "You have drawn:" << drawnNumber;
            return drawnNumber == luckyNumber;
        };

        const auto onDone = [](DoneWith result) {
            if (result == DoneWith::Success)
                qDebug() << "You have won! Congratulations!";
            else if (result == DoneWith::Error)
                qDebug() << "You have lost. Try again.";
        };

        const Group recipe {
            For (RepeatIterator(maxTries)) >> Do {
                stopOnSuccess,
                timeoutTask(1s),
                QSyncTask(onNumberCheck)
            },
            onGroupDone(onDone)
        };
    \endcode

    The possible output when the \c recipe is started by the QTaskTree,
    and finishes with success:
    \code
        Today's lucky number is: 6
        You have 5 lottery tickets.
        Let's start the drawing...
        You have drawn: 9
        You have drawn: 6
        You have won! Congratulations!
    \endcode

    In case it finishes with an error, the output might look like:
    \code
        Today's lucky number is: 8
        You have 5 lottery tickets.
        Let's start the drawing...
        You have drawn: 1
        You have drawn: 7
        You have drawn: 6
        You have drawn: 7
        You have drawn: 2
        You have lost. Try again.
    \endcode
*/
RepeatIterator::RepeatIterator(int count) : Iterator(count) {}

/*!
    \class QtTaskTree::UntilIterator
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief The conditional iterator to be used inside For element.
    \reentrant

    \sa Iterator, ForeverIterator, RepeatIterator, ListIterator
*/

/*!
    Constructs the conditional iterator for the
    For (UntilIterator(condition)) >> Do {} construct.
    The iterator will repeat until passed \a condition returns true.

    Example usage:

    \code
        static const int maxNumber = 10;
        const int luckyNumber = QRandomGenerator().global()->bounded(maxNumber);

        qDebug() << "Today's lucky number is:" << luckyNumber;
        qDebug() << "Let's start the drawing...";

        const auto onConditionCheck = [luckyNumber](int iteration) {
            if (iteration == 0)
                return true;
            const int drawnNumber = QRandomGenerator().global()->bounded(maxNumber);
            qDebug() << "You have drawn:" << drawnNumber;
            bool won = drawnNumber == luckyNumber;
            if (won)
                qDebug() << "You have won! Congratulations!";
            return !won;
        };

        const Group recipe = For (UntilIterator(onConditionCheck)) >> Do {
            timeoutTask(1s, DoneResult::Success)
        };
    \endcode

    The possible output when the \c recipe is started by the QTaskTree:
    \code
        Today's lucky number is: 7
        Let's start the drawing...
        You have drawn: 2
        You have drawn: 5
        You have drawn: 0
        You have drawn: 2
        You have drawn: 4
        You have drawn: 9
        You have drawn: 9
        You have drawn: 7
        You have won! Congratulations!
    \endcode
*/
UntilIterator::UntilIterator(const Condition &condition) : Iterator(condition) {}

/*!
    \class QtTaskTree::ListIterator
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief The list iterator to be used inside For element.
    \reentrant

    \sa Iterator, ForeverIterator, RepeatIterator, UntilIterator
*/

/*!
    \fn template <typename T> ListIterator<T>::ListIterator<T>(const QList<T> &list)

    Constructs the list iterator for the
    For (ListIterator(list)) >> Do {} construct.
    The iterator will iterate over each element from the passed \a list.

    \sa Iterator::iteration()
*/

/*!
    \fn template <typename T> const T *ListIterator<T>::operator->() const

    Returns a pointer to the current element inside a Do body.
    Use this function only from inside the handler body of any
    GroupItem element placed in the QtTaskTree::Do body of the recipe,
    otherwise you may expect a crash.
    Make sure that ListIterator is passed to the For element.
*/

/*!
    \fn template <typename T> const T &ListIterator<T>::operator*() const

    Returns a reference to the current element inside a Do body.
    Use this function only from inside the handler body of any
    GroupItem element placed in the QtTaskTree::Do body of the recipe,
    otherwise you may expect a crash.
    Make sure that ListIterator is passed to the For element.
*/

using StoragePtr = void *;

static constexpr QLatin1StringView s_activeStorageWarning =
    "The referenced storage is not reachable in the running tree. "
    "A nullptr will be returned which might lead to a crash in the calling code. "
    "It is possible that no storage was added to the tree, "
    "or the storage is not reachable from where it is referenced."_L1;

class StorageThreadData
{
    Q_DISABLE_COPY_MOVE(StorageThreadData)

public:
    StorageThreadData() = default;
    void pushStorage(StoragePtr storagePtr)
    {
        m_activeStorageStack.push_back({storagePtr, activeTaskTree()});
    }
    void popStorage()
    {
        QT_TASKTREE_ASSERT(m_activeStorageStack.size(), return);
        m_activeStorageStack.pop_back();
    }
    StoragePtr activeStorage() const
    {
        QT_TASKTREE_ASSERT(m_activeStorageStack.size(),
                  qWarning().noquote() << s_activeStorageWarning; return nullptr);
        const QPair<StoragePtr, QTaskTree *> &top = m_activeStorageStack.last();
        QT_TASKTREE_ASSERT(top.second == activeTaskTree(),
                  qWarning().noquote() << s_activeStorageWarning; return nullptr);
        return top.first;
    }

private:
    QList<QPair<StoragePtr, QTaskTree *>> m_activeStorageStack;
};

class StorageData
{
public:
    StorageThreadData &threadData() { return m_threadData.data(); }

    const StorageBase::StorageConstructor m_constructor = {};
    const StorageBase::StorageDestructor m_destructor = {};
    LocalThreadStorage<StorageThreadData> m_threadData = {};
};

StorageBase::StorageBase(const StorageConstructor &ctor, const StorageDestructor &dtor)
    : m_storageData(new StorageData{ctor, dtor})
{}

StoragePtr StorageBase::activeStorageVoid() const
{
    return m_storageData->threadData().activeStorage();
}

class GroupItemPrivate : public QSharedData
{
public:
    GroupItem::Type m_type = GroupItem::Type::Group;
    GroupItems m_children;
    GroupItem::GroupData m_groupData;
    QList<StorageBase> m_storageList;
    GroupItem::TaskHandler m_taskHandler;
};

/*!
    Constructs a GroupItem element with a given list of \a children.

    When this GroupItem element is parsed by the QTaskTree,
    it is simply replaced with its \a children.

    This constructor is useful when constructing a
    \l {QtTaskTree::} {Group} element with lists of
    GroupItem elements:

    \code
        static GroupItems getItems();

        ...

        const Group root {
            parallel,
            finishAllAndSuccess,
            getItems(), // GroupItems list is wrapped into a single GroupItem element
            onGroupSetup(...),
            onGroupDone(...)
        };
    \endcode

    If you want to create a subtree, use \l {QtTaskTree::} {Group} instead.

    \note Don't confuse this GroupItem with the
          \l {QtTaskTree::} {Group} element, as
          \l {QtTaskTree::} {Group} keeps its children nested
          after being parsed by the task tree, while this GroupItem does not.

    \sa {QtTaskTree::} {Group}
*/
GroupItem::GroupItem(const GroupItems &children)
    : GroupItem(Type::List)
{
    addChildren(children);
}

/*!
    \overload
    \sa GroupItem(const QtTaskTree::GroupItems &children)
*/
GroupItem::GroupItem(std::initializer_list<GroupItem> children)
    : GroupItem(Type::List)
{
    addChildren(children);
}

GroupItem::~GroupItem() = default;
GroupItem::GroupItem(const GroupItem &other) = default;
GroupItem::GroupItem(GroupItem &&other) noexcept = default;
GroupItem &GroupItem::operator=(const GroupItem &other) = default;

GroupItem::GroupItem(const Iterator &loop)
    : GroupItem(GroupData{{}, {}, {}, loop})
{}

GroupItem::GroupItem()
    : d(new GroupItemPrivate)
{}

GroupItem::GroupItem(Type type)
    : GroupItem()
{
    d->m_type = type;
}

GroupItem::GroupItem(const GroupData &data)
    : GroupItem(Type::GroupData)
{
    d->m_groupData = data;
}

GroupItem::GroupItem(const TaskHandler &handler)
    : GroupItem(Type::TaskHandler)
{
    d->m_taskHandler = handler;
}

GroupItem::GroupItem(const StorageBase &storage)
    : GroupItem(Type::Storage)
{
    d->m_storageList = {storage};
}

void GroupItem::addChildren(const GroupItems &children)
{
    QT_TASKTREE_ASSERT(d->m_type == Type::Group || d->m_type == Type::List,
              qWarning("Only Group or List may have children, skipping..."); return);
    if (d->m_type == Type::List) {
        d->m_children.append(children);
        return;
    }
    for (const GroupItem &child : children) {
        switch (child.d->m_type) {
        case Type::List:
            addChildren(child.d->m_children);
            break;
        case Type::Group:
            d->m_children.append(child);
            break;
        case Type::GroupData:
            if (child.d->m_groupData.m_groupHandler.m_setupHandler) {
                QT_TASKTREE_ASSERT(!d->m_groupData.m_groupHandler.m_setupHandler,
                          qWarning("Group setup handler redefinition, overriding..."));
                d->m_groupData.m_groupHandler.m_setupHandler
                    = child.d->m_groupData.m_groupHandler.m_setupHandler;
            }
            if (child.d->m_groupData.m_groupHandler.m_doneHandler) {
                QT_TASKTREE_ASSERT(!d->m_groupData.m_groupHandler.m_doneHandler,
                          qWarning("Group done handler redefinition, overriding..."));
                d->m_groupData.m_groupHandler.m_doneHandler
                    = child.d->m_groupData.m_groupHandler.m_doneHandler;
                d->m_groupData.m_groupHandler.m_callDoneFlags
                    = child.d->m_groupData.m_groupHandler.m_callDoneFlags;
            }
            if (child.d->m_groupData.m_parallelLimit) {
                QT_TASKTREE_ASSERT(!d->m_groupData.m_parallelLimit,
                          qWarning("Group execution mode redefinition, overriding..."));
                d->m_groupData.m_parallelLimit = child.d->m_groupData.m_parallelLimit;
            }
            if (child.d->m_groupData.m_workflowPolicy) {
                QT_TASKTREE_ASSERT(!d->m_groupData.m_workflowPolicy,
                          qWarning("Group workflow policy redefinition, overriding..."));
                d->m_groupData.m_workflowPolicy = child.d->m_groupData.m_workflowPolicy;
            }
            if (child.d->m_groupData.m_iterator) {
                QT_TASKTREE_ASSERT(!d->m_groupData.m_iterator,
                          qWarning("Group loop redefinition, overriding..."));
                d->m_groupData.m_iterator = child.d->m_groupData.m_iterator;
            }
            break;
        case Type::TaskHandler:
            QT_TASKTREE_ASSERT(child.d->m_taskHandler.m_taskAdapterConstructor,
                      qWarning("Task create handler can't be null, skipping..."); return);
            d->m_children.append(child);
            break;
        case Type::Storage:
            // Check for duplicates, as can't have the same storage twice on the same level.
            for (const StorageBase &storage : child.d->m_storageList) {
                if (d->m_storageList.contains(storage)) {
                    QT_TASKTREE_ASSERT(false, qWarning("Can't add the same storage into one Group twice, "
                                              "skipping..."));
                    continue;
                }
                d->m_storageList.append(storage);
            }
            break;
        }
    }
}

GroupItem GroupItem::groupHandler(const GroupHandler &handler)
{
    return GroupItem({handler});
}

GroupItem::TaskHandler GroupItem::taskHandler() const
{
    return d->m_taskHandler;
}

/*!
    \class QtTaskTree::ExecutableItem
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief Base class for executable task items.
    \reentrant

    ExecutableItem provides an additional interface for items
    containing executable tasks. Use withTimeout() to attach a timeout
    to a task. Use withLog() to include debugging information about
    the task startup and the execution result.
*/

ExecutableItem::ExecutableItem() = default;

ExecutableItem::ExecutableItem(const TaskHandler &handler) : GroupItem(handler) {}

/*!
    Attaches QTimeoutTask to a copy of \c this ExecutableItem,
    elapsing after \a timeout in milliseconds, with an optionally provided
    timeout \a handler, and returns the coupled item.

    When the ExecutableItem finishes before \a timeout passes,
    the returned item finishes immediately with the task's result.
    Otherwise, \a handler is invoked (if provided),
    the task is canceled, and the returned item finishes with an error.
*/
Group ExecutableItem::withTimeout(milliseconds timeout,
                                  const std::function<void()> &handler) const
{
    const auto onSetup = [timeout](milliseconds &timeoutData) { timeoutData = timeout; };
    return Group {
        parallel,
        stopOnSuccessOrError,
        Group {
            finishAllAndError,
            handler ? QTimeoutTask(onSetup, [handler] { handler(); }, CallDone::OnSuccess)
                    : QTimeoutTask(onSetup)
        },
        *this
    };
}

static QString currentTime() { return QTime::currentTime().toString(Qt::ISODateWithMs); }

static QString logHeader(const QString &logName)
{
    return QString::fromLatin1("TASK TREE LOG [%1] \"%2\"").arg(currentTime(), logName);
};

/*!
    Attaches a custom debug printout to a copy of \c this ExecutableItem,
    issued on task startup and after the task is finished,
    and returns the coupled item.

    The debug printout includes a timestamp of the event (start or finish)
    and \a logName to identify the specific task in the debug log.

    The finish printout contains the additional information whether
    the execution was synchronous or asynchronous,
    its result (the value described by the DoneWith enum),
    and the total execution time in milliseconds.
*/
Group ExecutableItem::withLog(const QString &logName) const
{
    struct LogStorage
    {
        time_point<system_clock, nanoseconds> start;
        int asyncCount = 0;
    };
    const Storage<LogStorage> storage;
    return Group {
        storage,
        onGroupSetup([storage, logName] {
            storage->start = system_clock::now();
            storage->asyncCount = activeTaskTree()->asyncCount();
            qDebug().noquote().nospace() << logHeader(logName) << " started.";
        }),
        *this,
        onGroupDone([storage, logName](DoneWith result) {
            const auto elapsed = duration_cast<milliseconds>(system_clock::now() - storage->start);
            const int asyncCountDiff = activeTaskTree()->asyncCount() - storage->asyncCount;
            QT_TASKTREE_CHECK(asyncCountDiff >= 0);
            const QMetaEnum doneWithEnum = QMetaEnum::fromType<DoneWith>();
            const QString syncType = asyncCountDiff ? QString::fromLatin1("asynchronously")
                                                    : QString::fromLatin1("synchronously");
            qDebug().noquote().nospace() << logHeader(logName) << " finished " << syncType
                                         << " with " << doneWithEnum.valueToKey(int(result))
                                         << " within " << elapsed.count() << "ms.";
        })
    };
}

/*!
    \fn Group ExecutableItem::operator!(const ExecutableItem &item)

    Returns a Group with the DoneResult of \a item negated.

    If \a item reports DoneResult::Success, the returned item reports
    DoneResult::Error. If \a item reports DoneResult::Error,
    the returned item reports DoneResult::Success.

    The returned item is equivalent to:
    \code
        Group {
            item,
            onGroupDone([](DoneWith doneWith) { return toDoneResult(doneWith == DoneWith::Error); })
        }
    \endcode

    \sa operator&&(), operator||()
*/
Group operator!(const ExecutableItem &item)
{
    return {
        item,
        onGroupDone([](DoneWith doneWith) { return toDoneResult(doneWith == DoneWith::Error); })
    };
}

/*!
    \fn Group ExecutableItem::operator&&(const ExecutableItem &first, const ExecutableItem &second)

    Returns a Group with \a first and \a second tasks merged with conjunction.

    Both \a first and \a second tasks execute in sequence. If both tasks
    report DoneResult::Success, the returned item reports DoneResult::Success.
    Otherwise, the returned item reports DoneResult::Error.

    The returned item is
    \l {https://en.wikipedia.org/wiki/Short-circuit_evaluation}{short-circuiting}:
    if the \a first task reports DoneResult::Error,
    the \a second task is skipped, and the returned item reports
    DoneResult::Error immediately.

    The returned item is equivalent to:
    \code
        Group { stopOnError, first, second }
    \endcode

    \note Parallel execution of conjunction in a short-circuit manner
          can be achieved with the following code:
          \c {Group { parallel, stopOnError, first, second }}. In this case:
          if the \e {first finished} task reports DoneResult::Error,
          the \e other task is canceled, and the group reports
          DoneResult::Error immediately.

    \sa operator||(), operator!()
*/
Group operator&&(const ExecutableItem &first, const ExecutableItem &second)
{
    return { stopOnError, first, second };
}

/*!
    \fn Group ExecutableItem::operator||(const ExecutableItem &first, const ExecutableItem &second)

    Returns a Group with \a first and \a second tasks merged with disjunction.

    Both \a first and \a second tasks execute in sequence. If both tasks
    report DoneResult::Error, the returned item reports DoneResult::Error.
    Otherwise, the returned item reports DoneResult::Success.

    The returned item is
    \l {https://en.wikipedia.org/wiki/Short-circuit_evaluation}{short-circuiting}:
    if the \a first task reports DoneResult::Success,
    the \a second task is skipped, and the returned item reports
    DoneResult::Success immediately.

    The returned item is equivalent to:
    \code
        Group { stopOnSuccess, first, second }
    \endcode

    \note Parallel execution of disjunction in a short-circuit manner
          can be achieved with the following code:
          \c {Group { parallel, stopOnSuccess, first, second }}. In this case
          if the \e {first finished} task reports DoneResult::Success,
          the \e other task is canceled, and the group reports
          DoneResult::Success immediately.

    \sa operator&&(), operator!()
*/
Group operator||(const ExecutableItem &first, const ExecutableItem &second)
{
    return { stopOnSuccess, first, second };
}

/*!
    \fn Group ExecutableItem::operator&&(const ExecutableItem &item, DoneResult result)
    \overload ExecutableItem::operator&&()

    Returns the \a item task if the \a result is DoneResult::Success;
    otherwise returns the \a item task with its done result tweaked
    to DoneResult::Error.

    The \c {task && DoneResult::Error} is an eqivalent to tweaking the
    task's done result into DoneResult::Error unconditionally.
*/
Group operator&&(const ExecutableItem &item, DoneResult result)
{
    return { result == DoneResult::Success ? stopOnError : finishAllAndError, item };
}

/*!
    \fn Group ExecutableItem::operator||(const ExecutableItem &item, DoneResult result)
    \overload ExecutableItem::operator||()

    Returns the \a item task if the \a result is DoneResult::Error;
    otherwise returns the \a item task with its done result tweaked to
    DoneResult::Success.

    The \c {task || DoneResult::Success} is an eqivalent to tweaking
    the task's done result into DoneResult::Success unconditionally.
*/
Group operator||(const ExecutableItem &item, DoneResult result)
{
    return { result == DoneResult::Error ? stopOnError : finishAllAndSuccess, item };
}

/*!
    \fn template <typename SenderSignalPairGetter> Group ExecutableItem::withCancel(SenderSignalPairGetter &&getter, std::initializer_list<GroupItem> postCancelRecipe = {}) const

    Makes a copy of \c this ExecutableItem cancelable.
    The passed \a getter is a function returning a
    \c std::pair<QObject *, PointerToMemberFunction> that describes
    the emitter and its cancellation signal. When the cancellation signal
    is emitted, \c this ExecutableItem is canceled, an optionally provided
    \a postCancelRecipe is executed, and returned Group finishes with an error.

    When \c this ExecutableItem finishes before the cancellation signal
    is emitted, the returned Group finishes immediately with
    the same result that \c this ExecutableItem finished.
    In this case the optionally provided \a postCancelRecipe is skipped.

    The connection to the cancellation signal is established
    when \c this ExecutableItem is about to be started. If the
    cancelation signal was triggered before, this won't be
    noticed after \c this ExecutableItem is started.
*/

Group ExecutableItem::withCancelImpl(
    const std::function<void(QObject *, const std::function<void()> &)> &connectWrapper,
    const GroupItems &postCancelRecipe) const
{
    const Storage<bool> canceledStorage(false);

    const auto onSetup = [connectWrapper, canceledStorage](QBarrier &barrier) {
        connectWrapper(&barrier, [barrierPtr = &barrier, canceled = canceledStorage.activeStorage()] {
            *canceled = true;
            barrierPtr->advance();
        });
    };

    const auto wasCanceled = [canceledStorage] { return *canceledStorage; };

    return {
        continueOnError,
        canceledStorage,
        Group {
            parallel,
            stopOnSuccessOrError,
            QBarrierTask(onSetup) && errorItem,
            *this
        },
        If (wasCanceled) >> Then {
            postCancelRecipe
        }
    };
}

/*!
    \fn template <typename SenderSignalPairGetter> Group ExecutableItem::withAccept(SenderSignalPairGetter &&getter) const

    Returns a copy of \c this ExecutableItem coupled with a signal awaiter.
    The passed \a getter is a function returning a
    \c std::pair<QObject *, PointerToMemberFunction>
    that describes the emitter and its awaiting signal.

    When \c this ExecutableItem finishes with an error, the returned
    \l {QtTaskTree::} {Group} finishes immediately with an error,
    without awaiting for the awaiter's signal.

    When \c this ExecutableItem finishes with success, the returned
    \l {QtTaskTree::} {Group} doesn't finish immediately but waits
    for the awaiting signal to be sent. After the awaiting signal
    is sent, the returned \l {QtTaskTree::} {Group} finishes with success.
    If the awaiting signal is sent before \c this ExecutableItem finishes,
    the awaiting phase is skipped and the returned
    \l {QtTaskTree::} {Group} finishes synchronously.

    The connection to the awaiter's signal is established
    when \c this ExecutableItem is about to be started. If the
    awaiting signal was triggered before, this won't be
    noticed after \c this ExecutableItem is started.
*/

Group ExecutableItem::withAcceptImpl(
    const std::function<void(QObject *, const std::function<void()> &)> &connectWrapper) const
{
    const auto onSetup = [connectWrapper](QBarrier &barrier) {
        connectWrapper(&barrier, [barrierPtr = &barrier] { barrierPtr->advance(); });
    };
    return Group {
        parallel,
        QBarrierTask(onSetup),
        *this
    };
}

/*!
    Constructs a group with a given list of \a children.

    This constructor is useful when the child items of the group
    are not known at compile time, but later, at runtime:

    \code
        const QStringList sourceList = ...;

        GroupItems groupItems { parallel };

        for (const QString &source : sourceList) {
            const QNetworkReplyWrapperTask task(...); // use source for setup handler
            groupItems << task;
        }

        const Group group(groupItems);
    \endcode
*/
Group::Group(const GroupItems &children)
{
    addChildren(children);
}

/*!
    Constructs a group from \c std::initializer_list given by \a children.

    This constructor is useful when all child items of the group
    are known at compile time:

    \code
        const Group group {
            finishAllAndSuccess,
            QNetworkReplyWrapperTask(...),
            Group {
                QNetworkReplyWrapperTask(...),
                Group {
                    parallel,
                    QNetworkReplyWrapperTask(...),
                    QNetworkReplyWrapperTask(...),
                }
                QThreadFunctionTask<QString>(...)
            }
        };
    \endcode
*/
Group::Group(std::initializer_list<GroupItem> children)
{
    addChildren(children);
}

class QTaskTreePrivate;
class TaskNode;
class RuntimeContainer;
class RuntimeIteration;
class RuntimeTask;

class ExecutionContextActivator
{
public:
    ExecutionContextActivator(RuntimeIteration *iteration) {
        activateTaskTree(iteration);
        activateContext(iteration);
    }
    ExecutionContextActivator(RuntimeContainer *container) {
        activateTaskTree(container);
        activateContext(container);
    }
    ~ExecutionContextActivator() {
        for (int i = m_activeStorages.size() - 1; i >= 0; --i) // iterate in reverse order
            m_activeStorages[i].m_storageData->threadData().popStorage();
        for (int i = m_activeIterators.size() - 1; i >= 0; --i) // iterate in reverse order
            m_activeIterators[i].m_iteratorData->threadData().popIteration();
        QT_TASKTREE_ASSERT(s_activeTaskTrees.size(), return);
        s_activeTaskTrees.pop_back();
    }

private:
    void activateTaskTree(RuntimeIteration *iteration);
    void activateTaskTree(RuntimeContainer *container);
    void activateContext(RuntimeIteration *iteration);
    void activateContext(RuntimeContainer *container);
    QList<Iterator> m_activeIterators;
    QList<StorageBase> m_activeStorages;
};

class ContainerNode
{
    Q_DISABLE_COPY(ContainerNode)

public:
    ContainerNode(ContainerNode &&other) = default;
    ContainerNode(QTaskTreePrivate *taskTreePrivate, const GroupItem &task);

    QTaskTreePrivate *const m_taskTreePrivate = nullptr;

    const GroupItem::GroupHandler m_groupHandler;
    const int m_parallelLimit = 1;
    const WorkflowPolicy m_workflowPolicy = WorkflowPolicy::StopOnError;
    const std::optional<Iterator> m_iterator;
    const QList<StorageBase> m_storageList;
    std::vector<TaskNode> m_children;
    const int m_taskCount = 0;
};

class TaskNode
{
    Q_DISABLE_COPY(TaskNode)

public:
    TaskNode(TaskNode &&other) = default;
    TaskNode(QTaskTreePrivate *taskTreePrivate, const GroupItem &task)
        : m_taskHandler(task.d->m_taskHandler)
        , m_container(taskTreePrivate, task)
    {}

    bool isTask() const { return bool(m_taskHandler.m_taskAdapterConstructor); }
    int taskCount() const { return isTask() ? 1 : m_container.m_taskCount; }

    const GroupItem::TaskHandler m_taskHandler;
    ContainerNode m_container;
};

class QTaskTreePrivate : public QObjectPrivate
{
    Q_DISABLE_COPY_MOVE(QTaskTreePrivate)

public:
    explicit QTaskTreePrivate(QTaskTree *taskTree);
    ~QTaskTreePrivate();

    void start();
    void stop();
    void bumpAsyncCount();
    void advanceProgress(int byValue);
    void emitDone(DoneWith result);
    void callSetupHandler(const StorageBase &storage, StoragePtr storagePtr) {
        callStorageHandler(storage, storagePtr, &StorageHandler::m_setupHandler);
    }
    void callDoneHandler(const StorageBase &storage, StoragePtr storagePtr) {
        callStorageHandler(storage, storagePtr, &StorageHandler::m_doneHandler);
    }
    struct StorageHandler {
        StorageBase::StorageHandler m_setupHandler = {};
        StorageBase::StorageHandler m_doneHandler = {};
    };
    typedef StorageBase::StorageHandler StorageHandler::*HandlerPtr; // ptr to class member
    void callStorageHandler(const StorageBase &storage, StoragePtr storagePtr, HandlerPtr ptr)
    {
        const auto it = m_storageHandlers.constFind(storage);
        if (it == m_storageHandlers.constEnd())
            return;
        const StorageHandler storageHandler = *it;
        if (storageHandler.*ptr) {
            GuardLocker locker(m_guard);
            (storageHandler.*ptr)(storagePtr);
        }
    }

    // Node related methods

    // If returned value != Continue, childDone() needs to be called in parent container (in caller)
    // in order to unwind properly.
    void startTask(const std::shared_ptr<RuntimeTask> &node);
    void stopTask(RuntimeTask *node);
    bool invokeTaskDoneHandler(RuntimeTask *node, DoneWith doneWith);

    // Container related methods

    void continueContainer(RuntimeContainer *container);
    void startChildren(RuntimeContainer *container);
    void childDone(RuntimeIteration *iteration, bool success);
    void stopContainer(RuntimeContainer *container);
    bool invokeDoneHandler(RuntimeContainer *container, DoneWith doneWith);
    bool invokeIteratorHandler(RuntimeContainer *container);

    template <typename Container, typename Handler, typename ...Args,
              typename ReturnType = std::invoke_result_t<Handler, Args...>>
    ReturnType invokeHandler(Container *container, Handler &&handler, Args &&...args)
    {
        QT_TASKTREE_ASSERT(!m_guard.isLocked(), qWarning("Nested execution of handlers detected. "
            "This may happen when one task's handler has entered a nested event loop, "
            "and other task finished during nested event loop's processing, "
            "causing stopping (canceling) the task executing the nested event loop. "
            "This includes the case when QCoreApplication::processEvents() was called from "
            "the handler. It may also happen when the QBarrier task is advanced directly "
            "from some other task handler. This will lead to a crash. "
            "Avoid event processing during handlers' execution. "
            "If it can't be avoided, make sure no other tasks are run in parallel when "
            "processing events from the handler."));
        ExecutionContextActivator activator(container);
        GuardLocker locker(m_guard);
        return std::invoke(std::forward<Handler>(handler), std::forward<Args>(args)...);
    }

    static int effectiveIteratorCount(const std::optional<Iterator> &iterator)
    {
        return iterator && iterator->m_iteratorData->m_loopCount ? *iterator->m_iteratorData->m_loopCount : 1;
    }

    QTaskTree *q = nullptr;
    Guard m_guard;
    int m_progressValue = 0;
    int m_asyncCount = 0;
    QSet<StorageBase> m_storages;
    QHash<StorageBase, StorageHandler> m_storageHandlers;
    std::optional<TaskNode> m_root;
    std::shared_ptr<RuntimeTask> m_runtimeRoot; // Keep me last in order to destruct first
};

static bool initialSuccessBit(WorkflowPolicy workflowPolicy)
{
    switch (workflowPolicy) {
    case WorkflowPolicy::StopOnError:
    case WorkflowPolicy::ContinueOnError:
    case WorkflowPolicy::FinishAllAndSuccess:
        return true;
    case WorkflowPolicy::StopOnSuccess:
    case WorkflowPolicy::ContinueOnSuccess:
    case WorkflowPolicy::StopOnSuccessOrError:
    case WorkflowPolicy::FinishAllAndError:
        return false;
    }
    QT_TASKTREE_CHECK(false);
    return false;
}

class RuntimeIteration
{
    Q_DISABLE_COPY(RuntimeIteration)

public:
    RuntimeIteration(int index, RuntimeContainer *container);
    ~RuntimeIteration();
    std::optional<Iterator> iterator() const;
    void removeChild(RuntimeTask *node);

    const int m_iterationIndex = 0;
    const bool m_isProgressive = true;
    RuntimeContainer *m_container = nullptr;
    int m_doneCount = 0;
    std::vector<std::shared_ptr<RuntimeTask>> m_children = {}; // Owning.
};

class RuntimeContainer
{
    Q_DISABLE_COPY(RuntimeContainer)

public:
    RuntimeContainer(const ContainerNode &taskContainer, RuntimeTask *parentTask)
        : m_containerNode(taskContainer)
        , m_parentTask(parentTask)
        , m_storages(createStorages(taskContainer))
        , m_successBit(initialSuccessBit(taskContainer.m_workflowPolicy))
        , m_shouldIterate(taskContainer.m_iterator)
    {}

    ~RuntimeContainer()
    {
        for (int i = m_containerNode.m_storageList.size() - 1; i >= 0; --i) { // iterate in reverse order
            const StorageBase storage = m_containerNode.m_storageList[i];
            StoragePtr storagePtr = m_storages.value(i);
            if (m_callStorageDoneHandlersOnDestruction)
                m_containerNode.m_taskTreePrivate->callDoneHandler(storage, storagePtr);
            storage.m_storageData->m_destructor(storagePtr);
        }
    }

    static QList<StoragePtr> createStorages(const ContainerNode &container);
    bool isStarting() const { return m_startGuard.isLocked(); }
    RuntimeIteration *parentIteration() const;
    bool updateSuccessBit(bool success);
    void deleteFinishedIterations();
    int progressiveIteratorCount() const
    {
        return m_containerNode.m_taskTreePrivate->effectiveIteratorCount(m_containerNode.m_iterator);
    }

    const ContainerNode &m_containerNode; // Not owning.
    RuntimeTask *m_parentTask = nullptr; // Not owning.
    const QList<StoragePtr> m_storages; // Owning.

    bool m_successBit = true;
    bool m_callStorageDoneHandlersOnDestruction = false;
    Guard m_startGuard;

    int m_iterationCount = 0;
    int m_nextToStart = 0;
    int m_runningChildren = 0;
    bool m_shouldIterate = true;
    std::vector<std::unique_ptr<RuntimeIteration>> m_iterations; // Owning.
};

class TaskInterfaceAdapter : public QObject
{
public:
    TaskInterfaceAdapter(const GroupItem::TaskHandler &taskHandler)
        : m_taskAdapter(taskHandler.m_taskAdapterConstructor())
        , m_taskAdapterDestructor(taskHandler.m_taskAdapterDestructor)
    {}
    ~TaskInterfaceAdapter() { m_taskAdapterDestructor(m_taskAdapter); }

    QTaskInterface m_taskInterface;
    GroupItem::TaskAdapterPtr m_taskAdapter = nullptr; // Owning.
    GroupItem::TaskAdapterDestructor m_taskAdapterDestructor;
};

class RuntimeTask
{
public:
    ~RuntimeTask()
    {
        if (m_taskInterfaceAdapter) {
            // Ensures the running task's d'tor doesn't emit done() signal. QTCREATORBUG-30204.
            QObject::disconnect(&m_taskInterfaceAdapter->m_taskInterface, &QTaskInterface::done, nullptr, nullptr);
        }
    }

    const TaskNode &m_taskNode; // Not owning.
    RuntimeIteration *m_parentIteration = nullptr; // Not owning.
    std::optional<RuntimeContainer> m_container = {}; // Owning.
    std::unique_ptr<TaskInterfaceAdapter> m_taskInterfaceAdapter = {}; // Owning.
    SetupResult m_setupResult = SetupResult::Continue;
};

RuntimeIteration::~RuntimeIteration() = default;

QTaskTreePrivate::QTaskTreePrivate(QTaskTree *taskTree)
    : q(taskTree) {}

QTaskTreePrivate::~QTaskTreePrivate() = default;

static bool isProgressive(RuntimeContainer *container)
{
    RuntimeIteration *iteration = container->m_parentTask->m_parentIteration;
    return iteration ? iteration->m_isProgressive : true;
}

void ExecutionContextActivator::activateTaskTree(RuntimeIteration *iteration)
{
    activateTaskTree(iteration->m_container);
}

void ExecutionContextActivator::activateTaskTree(RuntimeContainer *container)
{
    s_activeTaskTrees.push_back(container->m_containerNode.m_taskTreePrivate->q);
}

void ExecutionContextActivator::activateContext(RuntimeIteration *iteration)
{
    std::optional<Iterator> loop = iteration->iterator();
    if (loop) {
        loop->m_iteratorData->threadData().pushIteration(iteration->m_iterationIndex);
        m_activeIterators.append(*loop);
    }
    activateContext(iteration->m_container);
}

void ExecutionContextActivator::activateContext(RuntimeContainer *container)
{
    const ContainerNode &containerNode = container->m_containerNode;
    for (int i = 0; i < containerNode.m_storageList.size(); ++i) {
        const StorageBase &storage = containerNode.m_storageList[i];
        if (m_activeStorages.contains(storage))
            continue; // Storage shadowing: The storage is already active, skipping it...
        m_activeStorages.append(storage);
        storage.m_storageData->threadData().pushStorage(container->m_storages.value(i));
    }
    // Go to the parent after activating this storages so that storage shadowing works
    // in the direction from child to parent root.
    if (container->parentIteration())
        activateContext(container->parentIteration());
}

void QTaskTreePrivate::start()
{
    QT_TASKTREE_ASSERT(m_root, return);
    QT_TASKTREE_ASSERT(!m_runtimeRoot, return);
    m_asyncCount = 0;
    m_progressValue = 0;
    {
        GuardLocker locker(m_guard);
        Q_EMIT q->started();
        Q_EMIT q->asyncCountChanged(m_asyncCount);
        Q_EMIT q->progressValueChanged(m_progressValue);
    }
    // TODO: check storage handlers for not existing storages in tree
    for (auto it = m_storageHandlers.cbegin(); it != m_storageHandlers.cend(); ++it) {
        QT_TASKTREE_ASSERT(m_storages.contains(it.key()), qWarning("The registered storage doesn't "
                  "exist in task tree. Its handlers will never be called."));
    }
    m_runtimeRoot.reset(new RuntimeTask{*m_root});
    startTask(m_runtimeRoot);
    bumpAsyncCount();
}

void QTaskTreePrivate::stop()
{
    QT_TASKTREE_ASSERT(m_root, return);
    if (!m_runtimeRoot)
        return;
    stopTask(m_runtimeRoot.get());
    m_runtimeRoot.reset();
    emitDone(DoneWith::Cancel);
}

void QTaskTreePrivate::bumpAsyncCount()
{
    if (!m_runtimeRoot)
        return;
    ++m_asyncCount;
    GuardLocker locker(m_guard);
    Q_EMIT q->asyncCountChanged(m_asyncCount);
}

void QTaskTreePrivate::advanceProgress(int byValue)
{
    if (byValue == 0)
        return;
    QT_TASKTREE_CHECK(byValue > 0);
    QT_TASKTREE_CHECK(m_progressValue + byValue <= m_root->taskCount());
    m_progressValue += byValue;
    GuardLocker locker(m_guard);
    Q_EMIT q->progressValueChanged(m_progressValue);
}

void QTaskTreePrivate::emitDone(DoneWith result)
{
    QT_TASKTREE_CHECK(m_progressValue == m_root->taskCount());
    GuardLocker locker(m_guard);
    Q_EMIT q->done(result);
}

RuntimeIteration::RuntimeIteration(int index, RuntimeContainer *container)
    : m_iterationIndex(index)
    , m_isProgressive(index < container->progressiveIteratorCount() && isProgressive(container))
    , m_container(container)
{}

std::optional<Iterator> RuntimeIteration::iterator() const
{
    return m_container->m_containerNode.m_iterator;
}

void RuntimeIteration::removeChild(RuntimeTask *task)
{
    const auto it = std::find_if(m_children.cbegin(), m_children.cend(), [task](const auto &ptr) {
        return ptr.get() == task;
    });
    if (it != m_children.cend())
        m_children.erase(it);
}

static std::vector<TaskNode> createChildren(QTaskTreePrivate *taskTreePrivate,
                                            const GroupItems &children)
{
    std::vector<TaskNode> result;
    result.reserve(children.size());
    for (const GroupItem &child : children)
        result.emplace_back(taskTreePrivate, child);
    return result;
}

ContainerNode::ContainerNode(QTaskTreePrivate *taskTreePrivate, const GroupItem &task)
    : m_taskTreePrivate(taskTreePrivate)
    , m_groupHandler(task.d->m_groupData.m_groupHandler)
    , m_parallelLimit(task.d->m_groupData.m_parallelLimit.value_or(1))
    , m_workflowPolicy(task.d->m_groupData.m_workflowPolicy.value_or(WorkflowPolicy::StopOnError))
    , m_iterator(task.d->m_groupData.m_iterator)
    , m_storageList(task.d->m_storageList)
    , m_children(createChildren(taskTreePrivate, task.d->m_children))
    , m_taskCount(std::accumulate(m_children.cbegin(), m_children.cend(), 0,
                                  [](int r, const TaskNode &n) { return r + n.taskCount(); })
                  * taskTreePrivate->effectiveIteratorCount(m_iterator))
{
    for (const StorageBase &storage : m_storageList)
        m_taskTreePrivate->m_storages << storage;
}

QList<StoragePtr> RuntimeContainer::createStorages(const ContainerNode &container)
{
    QList<StoragePtr> storages;
    for (const StorageBase &storage : container.m_storageList) {
        StoragePtr storagePtr = storage.m_storageData->m_constructor();
        storages.append(storagePtr);
        container.m_taskTreePrivate->callSetupHandler(storage, storagePtr);
    }
    return storages;
}

RuntimeIteration *RuntimeContainer::parentIteration() const
{
    return m_parentTask->m_parentIteration;
}

bool RuntimeContainer::updateSuccessBit(bool success)
{
    if (m_containerNode.m_workflowPolicy == WorkflowPolicy::FinishAllAndSuccess
        || m_containerNode.m_workflowPolicy == WorkflowPolicy::FinishAllAndError
        || m_containerNode.m_workflowPolicy == WorkflowPolicy::StopOnSuccessOrError) {
        if (m_containerNode.m_workflowPolicy == WorkflowPolicy::StopOnSuccessOrError)
            m_successBit = success;
        return m_successBit;
    }

    const bool donePolicy = m_containerNode.m_workflowPolicy == WorkflowPolicy::StopOnSuccess
                         || m_containerNode.m_workflowPolicy == WorkflowPolicy::ContinueOnSuccess;
    m_successBit = donePolicy ? (m_successBit || success) : (m_successBit && success);
    return m_successBit;
}

void RuntimeContainer::deleteFinishedIterations()
{
    for (auto it = m_iterations.cbegin(); it != m_iterations.cend(); ) {
        if (it->get()->m_doneCount == int(m_containerNode.m_children.size()))
            it = m_iterations.erase(it);
        else
            ++it;
    }
}

void QTaskTreePrivate::continueContainer(RuntimeContainer *container)
{
    RuntimeTask *parentTask = container->m_parentTask;
    if (parentTask->m_setupResult == SetupResult::Continue)
        startChildren(container);
    if (parentTask->m_setupResult == SetupResult::Continue)
        return;

    const bool bit = container->updateSuccessBit(parentTask->m_setupResult == SetupResult::StopWithSuccess);
    RuntimeIteration *parentIteration = container->parentIteration();
    QT_TASKTREE_CHECK(parentTask);
    const bool result = invokeDoneHandler(container, bit ? DoneWith::Success : DoneWith::Error);
    parentTask->m_setupResult = toSetupResult(result);
    if (parentIteration) {
        parentIteration->removeChild(parentTask);
        if (!parentIteration->m_container->isStarting())
            childDone(parentIteration, result);
    } else {
        QT_TASKTREE_CHECK(m_runtimeRoot.get() == parentTask);
        m_runtimeRoot.reset();
        emitDone(result ? DoneWith::Success : DoneWith::Error);
    }
}

void QTaskTreePrivate::startChildren(RuntimeContainer *container)
{
    const ContainerNode &containerNode = container->m_containerNode;
    const int childCount = int(containerNode.m_children.size());

    GuardLocker locker(container->m_startGuard);

    while (containerNode.m_parallelLimit == 0
           || container->m_runningChildren < containerNode.m_parallelLimit) {
        container->deleteFinishedIterations();
        const bool firstIteration = container->m_iterationCount == 0;
        if (firstIteration || container->m_nextToStart == childCount) {
            const bool skipHandler = firstIteration && !container->m_shouldIterate;
            if (skipHandler || invokeIteratorHandler(container)) {
                container->m_nextToStart = 0;
                if (containerNode.m_children.size() > 0) {
                    container->m_iterations.emplace_back(
                        std::make_unique<RuntimeIteration>(container->m_iterationCount, container));
                }
                ++container->m_iterationCount;
            } else {
                if (container->m_iterations.empty()) {
                    if (firstIteration && isProgressive(container))
                        advanceProgress(containerNode.m_taskCount);
                    container->m_parentTask->m_setupResult = toSetupResult(container->m_successBit);
                }
                return;
            }
        }
        if (containerNode.m_children.size() == 0) // Empty loop body.
            continue;

        RuntimeIteration *iteration = container->m_iterations.back().get();
        const std::shared_ptr<RuntimeTask> task(
            new RuntimeTask{containerNode.m_children.at(container->m_nextToStart), iteration});
        iteration->m_children.emplace_back(task);
        ++container->m_runningChildren;
        ++container->m_nextToStart;

        startTask(task);
        if (task->m_setupResult == SetupResult::Continue)
            continue;

        task->m_parentIteration->removeChild(task.get());
        childDone(iteration, task->m_setupResult == SetupResult::StopWithSuccess);
        if (container->m_parentTask->m_setupResult != SetupResult::Continue)
            return;
    }
}

void QTaskTreePrivate::childDone(RuntimeIteration *iteration, bool success)
{
    RuntimeContainer *container = iteration->m_container;
    const WorkflowPolicy &workflowPolicy = container->m_containerNode.m_workflowPolicy;
    const bool shouldStop = workflowPolicy == WorkflowPolicy::StopOnSuccessOrError
                        || (workflowPolicy == WorkflowPolicy::StopOnSuccess && success)
                        || (workflowPolicy == WorkflowPolicy::StopOnError && !success);
    ++iteration->m_doneCount;
    --container->m_runningChildren;
    const bool updatedSuccess = container->updateSuccessBit(success);
    container->m_parentTask->m_setupResult = shouldStop ? toSetupResult(updatedSuccess) : SetupResult::Continue;
    if (shouldStop)
        stopContainer(container);

    if (container->isStarting())
        return;
    continueContainer(container);
}

void QTaskTreePrivate::stopContainer(RuntimeContainer *container)
{
    const ContainerNode &containerNode = container->m_containerNode;
    for (auto &iteration : container->m_iterations) {
        for (auto &child : iteration->m_children) {
            ++iteration->m_doneCount;
            stopTask(child.get());
        }

        if (iteration->m_isProgressive) {
            int skippedTaskCount = 0;
            for (int i = iteration->m_doneCount; i < int(containerNode.m_children.size()); ++i)
                skippedTaskCount += containerNode.m_children.at(i).taskCount();
            advanceProgress(skippedTaskCount);
        }
    }
    const int skippedIterations = container->progressiveIteratorCount() - container->m_iterationCount;
    if (skippedIterations > 0) {
        advanceProgress(container->m_containerNode.m_taskCount / container->progressiveIteratorCount()
                        * skippedIterations);
    }
}

static CallDone toCallDone(DoneWith result)
{
    switch (result) {
    case DoneWith::Success: return CallDone::OnSuccess;
    case DoneWith::Error: return CallDone::OnError;
    case DoneWith::Cancel: return CallDone::OnCancel;
    }
    return CallDone::Never;
}

bool shouldCallDone(CallDoneFlags callDone, DoneWith result)
{
    return callDone & toCallDone(result);
}

bool QTaskTreePrivate::invokeDoneHandler(RuntimeContainer *container, DoneWith doneWith)
{
    DoneResult result = toDoneResult(doneWith);
    const GroupItem::GroupHandler &groupHandler = container->m_containerNode.m_groupHandler;
    if (groupHandler.m_doneHandler && shouldCallDone(groupHandler.m_callDoneFlags, doneWith))
        result = invokeHandler(container, groupHandler.m_doneHandler, doneWith);
    container->m_callStorageDoneHandlersOnDestruction = true;
    return result == DoneResult::Success;
}

bool QTaskTreePrivate::invokeIteratorHandler(RuntimeContainer *container)
{
    if (container->m_shouldIterate) {
        const IteratorData *loopData = container->m_containerNode.m_iterator->m_iteratorData.get();
        if (loopData->m_loopCount) {
            container->m_shouldIterate = container->m_iterationCount < loopData->m_loopCount;
        } else if (loopData->m_condition) {
            container->m_shouldIterate = invokeHandler(container, loopData->m_condition,
                                                       container->m_iterationCount);
        }
    }
    return container->m_shouldIterate;
}

void QTaskTreePrivate::startTask(const std::shared_ptr<RuntimeTask> &node)
{
    if (!node->m_taskNode.isTask()) {
        const ContainerNode &containerNode = node->m_taskNode.m_container;
        node->m_container.emplace(containerNode, node.get());
        RuntimeContainer *container = &*node->m_container;
        if (containerNode.m_groupHandler.m_setupHandler) {
            container->m_parentTask->m_setupResult = invokeHandler(container, containerNode.m_groupHandler.m_setupHandler);
            if (container->m_parentTask->m_setupResult != SetupResult::Continue) {
                if (isProgressive(container))
                    advanceProgress(containerNode.m_taskCount);
                // Non-Continue SetupResult takes precedence over the workflow policy.
                container->m_successBit = container->m_parentTask->m_setupResult == SetupResult::StopWithSuccess;
            }
        }
        continueContainer(container);
        return;
    }

    const GroupItem::TaskHandler &handler = node->m_taskNode.m_taskHandler;
    node->m_taskInterfaceAdapter.reset(new TaskInterfaceAdapter(handler));
    node->m_setupResult = handler.m_taskAdapterSetupHandler
        ? invokeHandler(node->m_parentIteration, handler.m_taskAdapterSetupHandler, node->m_taskInterfaceAdapter->m_taskAdapter)
        : SetupResult::Continue;
    if (node->m_setupResult != SetupResult::Continue) {
        if (node->m_parentIteration->m_isProgressive)
            advanceProgress(1);
        node->m_parentIteration->removeChild(node.get());
        return;
    }
    QObject::connect(&node->m_taskInterfaceAdapter->m_taskInterface, &QTaskInterface::done,
                     q, [this, node](DoneResult doneResult) {
        const bool result = invokeTaskDoneHandler(node.get(), toDoneWith(doneResult));
        node->m_setupResult = toSetupResult(result);
        node->m_taskInterfaceAdapter.release()->deleteLater();
        RuntimeIteration *parentIteration = node->m_parentIteration;
        if (parentIteration->m_container->isStarting())
            return;

        parentIteration->removeChild(node.get());
        childDone(parentIteration, result);
        bumpAsyncCount();
    }, Qt::SingleShotConnection);
    handler.m_taskAdapterStarter(node->m_taskInterfaceAdapter->m_taskAdapter,
                                 &node->m_taskInterfaceAdapter->m_taskInterface);
}

void QTaskTreePrivate::stopTask(RuntimeTask *node)
{
    if (node->m_taskInterfaceAdapter) {
        invokeTaskDoneHandler(node, DoneWith::Cancel);
        node->m_taskInterfaceAdapter.reset();
        return;
    }

    if (!node->m_container)
        return;

    stopContainer(&*node->m_container);
    node->m_container->updateSuccessBit(false);
    invokeDoneHandler(&*node->m_container, DoneWith::Cancel);
}

bool QTaskTreePrivate::invokeTaskDoneHandler(RuntimeTask *node, DoneWith doneWith)
{
    DoneResult result = toDoneResult(doneWith);
    const GroupItem::TaskHandler &handler = node->m_taskNode.m_taskHandler;
    if (handler.m_taskAdapterDoneHandler && shouldCallDone(handler.m_callDoneFlags, doneWith)) {
        result = invokeHandler(node->m_parentIteration, handler.m_taskAdapterDoneHandler,
                               node->m_taskInterfaceAdapter->m_taskAdapter, doneWith);
    }
    if (node->m_parentIteration->m_isProgressive)
        advanceProgress(1);
    return result == DoneResult::Success;
}

} // namespace QtTaskTree

using namespace QtTaskTree;

/*!
    \class QSyncTask
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief Synchronously executes a custom handler between other tasks.
    \reentrant

    QSyncTask is useful when you want to execute an additional
    handler between other tasks. QSyncTask is seen by its parent
    \l {QtTaskTree::} {Group} as any other task.
    Avoid long-running execution of the QSyncTask's handler body,
    since it is executed synchronously from the caller thread.
    If that is unavoidable, consider using QThreadFunctionTask instead.
*/

/*!
    \fn template <typename Handler> QSyncTask::QSyncTask(Handler &&handler)

    Constructs an element that executes a passed \a handler synchronously.
    The \c Handler is of the \c std::function<DoneResult()> type.
    The \l {QtTaskTree::} {DoneResult} value,
    returned by the \a handler, is considered during parent group's
    \l {QtTaskTree::WorkflowPolicy} {workflow policy} resolution.
    Optionally, the shortened form of \c std::function<void()> is also accepted.
    In this case, it's assumed that the return value is
    \l {QtTaskTree::} {DoneResult::Success}.

    The passed \a handler executes synchronously from the caller thread,
    so avoid a long-running execution of the handler body.
    Otherwise, consider using QThreadFunctionTask.

    \note The QSyncTask element is not counted as a task when reporting
          task tree progress, and is not included in QTaskTree::taskCount()
          or QTaskTree::progressMaximum().
*/

/*!
    \class QDefaultTaskAdapter
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief A class template providing default task adapter used in QCustomTask.
    \reentrant

    The QDefaultTaskAdapter adapting Task may be used when the following
    conditions are met:

    \list 1
        \li The Task is derived from QObject.
        \li The Task has public start() method that starts the task.
        \li The Task emits done(QtTaskTree::DoneResult) or
            done(bool) signal when it's finished.
    \endlist

    If any of these conditions isn't met, you should provide your
    custom Adapter type adapting the Task type, to the
    QCustomTask<Task, Adapter> class template.
*/

/*!
    \class QCustomTask
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief A class template used for declaring custom task items and
           defining their setup and done handlers.
    \reentrant

    Describes custom task items within task tree recipes.

    Custom task names are aliased with unique names using the
    QCustomTask template with a given Task, Adapter and Deleter.
    For example, QThreadFunctionTask<T> is an alias to the QCustomTask
    that is defined to work with QThreadFunction<T> as an associated task class.
    The following table contains custom tasks provided by the TaskTree library
    and their associated task classes:

    \table
    \header
        \li Aliased Task Name
        \li Associated Task Class
        \li Brief Description
    \row
        \li QBarrierTask
        \li QBarrier
        \li Starts an awaiter task.
    \row
        \li QThreadFunctionTask<ReturnType>
        \li QThreadFunction<ReturnType>
        \li Starts an asynchronous task. Runs in a separate thread.
    \row
        \li QProcessTask
        \li QProcess
        \li Starts a process.
    \row
        \li QNetworkReplyWrapperTask
        \li QNetworkReplyWrapper
        \li Sends a network query.
    \row
        \li QTaskTreeTask
        \li QTaskTree
        \li Starts a nested task tree.
    \row
        \li QTcpSocketWrapperTask
        \li QTcpSocketWrapper
        \li Establishes TCP connection.
    \row
        \li QTimeoutTask
        \li \c std::chrono::milliseconds
        \li Starts a timer.
    \endtable

    The \c Task template parameter is mandatory, and specifies what type of
    \c Task the running QTaskTree will instantiate when it's a part of recipe.
    The \c Task type needs to be default constructible.

    The \c Adapter template argument is optional. It may be skipped
    if QDefaultTaskAdapter<Task> is fine to adapt the \c Task type.
    Otherwise, the \c Adapter for certain \c Task needs to have the following
    form:

    \code
    class Adapter
    {
    public:
        void operator()(Task *task, QTaskInterface *iface) { ... }
    };
    \endcode

    Implement the operator() above to start the \a task, and call
    QTaskInterface::reportDone() method on the passed \a iface when
    the task is finished.

    It is guaranteed the passed \c Task and QTaskInterface outlives the
    \c Adapter. If necessary, the destructor of \c Adapter may still
    access the passed \c Task.

    For more details see QTaskInterface.

    The \c Deleter template argument is optional. By default,
    the \c std::default_delete<Task> is used.
    The custom \c Deleter is useful when the destructor of the running \c Task
    may potentially block the caller thread. Instead of blocking,
    the custom deleter may move the running task into a separate thread
    and implement the blocking destruction there.
    In this way, the fast destruction (seen from the caller thread)
    of the running task with a blocking destructor may be achieved.

    \sa QTaskInterface
*/

/*!
    \typealias QCustomTask::TaskSetupHandler

    Type alias for \c std::function<SetupResult(Task &)>.

    TaskSetupHandler is an optional argument of a custom task
    element's constructor. Any function with the above signature,
    when passed as a task setup handler, will be called by the running
    task tree after the task is created and before it is started.

    Inside the body of the handler, you may configure the task
    according to your needs. The additional parameters, including storages,
    may be passed to the handler via the lambda capture.
    You can decide dynamically whether the task should be started
    or skipped with success or an error.

    \note Do not start the task inside the start handler by yourself.
    Leave it for QTaskTree, otherwise the behavior is undefined.
    QTaskTree already knows how to start the task thanks
    to the Adapter template parameter passed to the
    QCustomTask<Task, Adapter, Deleter> constructor.

    The return value of the handler instructs the running task tree
    on how to proceed after the handler's invocation is finished.
    The return value of \l {QtTaskTree::} {SetupResult::Continue}
    instructs the task tree to continue running,
    that is, to execute the associated \c Task. The return value of
    \l {QtTaskTree::} {SetupResult::StopWithSuccess}
    or \l {QtTaskTree::} {SetupResult::StopWithError}
    instructs the task tree to skip the task's execution and finish it
    immediately with success or an error, respectively.

    When the return type is either
    \l {QtTaskTree::} {SetupResult::StopWithSuccess}
    or \l {QtTaskTree::} {SetupResult::StopWithError},
    the task's done handler (if provided) isn't called afterwards.

    The constructor of a custom task accepts also functions in the shortened
    form of \c std::function<void(Task &)>, that is, the return value
    is \c void. In this case, it's assumed that the return value is
    \l {QtTaskTree::} {SetupResult::Continue}.

    \sa QCustomTask(), TaskDoneHandler, GroupSetupHandler
*/

/*!
    \typealias QCustomTask::TaskDoneHandler

    Type alias for
    \c std::function<QtTaskTree::DoneResult(const Task &, QtTaskTree::DoneWith)>
    or \l {QtTaskTree::} {DoneResult}.

    The TaskDoneHandler is an optional argument of a custom task element's
    constructor. Any function with the above signature, when passed as a
    task done handler, will be called by the running task tree after
    the task execution finished and before the final result of the execution
    is reported back to the parent group.

    Inside the body of the handler you may retrieve the final data from
    the finished task. The additional parameters, including storages,
    may be passed to the handler via the lambda capture.
    It is also possible to decide dynamically whether the task should
    finish with its return value, or the final result should be tweaked.

    The DoneWith argument is optional and your done handler may omit it.
    When provided, it holds the info about the final result of a task
    that will be reported to its parent.

    If you do not plan to read any data from the finished task,
    you may omit the \c {const Task &} argument.

    The returned \l {QtTaskTree::} {DoneResult} value is optional
    and your handler may return \c void instead. In this case,
    the final result of the task will be equal to the value indicated by
    the \l {QtTaskTree::} {DoneResult} argument.
    When the handler returns the \l {QtTaskTree::} {DoneResult}
    value, the task's final result may be tweaked inside
    the done handler's body by the returned value.

    For a TaskDoneHandler of the \l {QtTaskTree::} {DoneResult} type,
    no additional handling is executed, and the task finishes unconditionally
    with the passed value of \l {QtTaskTree::} {DoneResult}.

    \sa QCustomTask(), TaskSetupHandler, GroupDoneHandler
*/

/*!
    \fn template <typename Task, typename Adapter = QDefaultTaskAdapter<Task>, typename Deleter = std::default_delete<Task>> template <typename SetupHandler = QCustomTask::TaskSetupHandler, typename DoneHandler = QCustomTask::TaskDoneHandler> QCustomTask<Task, Adapter, Deleter>::QCustomTask<Task, Adapter, Deleter>(SetupHandler &&setup = QCustomTask::TaskSetupHandler(), DoneHandler &&done = QCustomTask::TaskDoneHandler(), QtTaskTree::CallDoneFlags callDone = QtTaskTree::CallDone::Always)

    Constructs a QCustomTask instance and attaches the \a setup and \a done
    handlers to the task. When the running task tree is about to start the task,
    it instantiates the associated \c Task object, invokes \a setup handler
    with a \e reference to the created task, and starts it.
    When the running task finishes, the task tree invokes a \a done handler,
    with a \c const \e reference to the created task.

    The passed \a setup handler is of the \l TaskSetupHandler type.
    For example:

    \code
        static void parseAndLog(const QString &input);

        ...

        const QString input = ...;

        const auto onFirstSetup = [input](QThreadFunction<void> &task) {
            if (input == "Skip")
                return SetupResult::StopWithSuccess; // This task won't start, the next one will
            if (input == "Error")
                return SetupResult::StopWithError; // This task and the next one won't start
            task.setThreadFunctionData(parseAndLog, input);
            // This task will start, and the next one will start after this one finished with success
            return SetupResult::Continue;
        };

        const auto onSecondSetup = [input](QThreadFunction<void> &task) {
            task.setThreadFunctionData(parseAndLog, input);
        };

        const Group group {
            QThreadFunctionTask<void>(onFirstSetup),
            QThreadFunctionTask<void>(onSecondSetup)
        };
    \endcode

    The \a done handler is of the \l TaskDoneHandler type.
    By default, the \a done handler is invoked whenever the task finishes.
    Pass a non-default value for the \a callDone argument when you want
    the handler to be called only on a successful, failed,
    or canceled execution.

    \sa TaskSetupHandler, TaskDoneHandler
*/

/*!
    \class QTaskInterface
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief QTaskInterface is a helper class used when adapting custom
           task's interface.
    \reentrant

    The custom \c Adapter type of the
    QCustomTask<\c Task, \c Adapter, \c Deleter>
    template is expected to have the following form,
    when adapting Worker task:

    \code
    class WorkerTaskAdapter
    {
    public:
        void operator()(Worker *task, QTaskInterface *iface) { ... }
    };

    using WorkerTask = QCustomTask<Worker, WorkerTaskAdapter>;
    \endcode
*/

QTaskInterface::QTaskInterface(QObject *parent) : QObject(parent) {}

/*!
    This method should be called when the task adapted via custom adapter
    is finished, passing the \a result of the task execution.

    Assuming the Worker emits a finished(bool) signal,
    the adapter may look like:

    \code
    class WorkerTaskAdapter
    {
    public:
        void operator()(Worker *task, QTaskInterface *iface) {
            connect(task, &Worker::finished, iface, [iface](bool success) {
                iface->reportDone(QtTaskTree::toDoneResult(success));
            });
            task->execute();
        }
    };

    using WorkerTask = QCustomTask<Worker, WorkerTaskAdapter>;
    \endcode

    \sa QCustomTask
*/

void QTaskInterface::reportDone(DoneResult result)
{
    Q_EMIT done(result, QPrivateSignal());
}

/*!
    \class QTaskTree
    \inheaderfile qtasktree.h
    \inmodule TaskTree
    \brief The QTaskTree class runs the tree of asynchronous tasks
           defined in a declarative way.
    \reentrant

    The description on what the task tree should do is passed via recipe.
*/

/*!
    Constructs an empty task tree with a given \a parent.
    Use setRecipe() to pass a declarative description on how the task tree
    should execute the tasks and how it should handle the finished tasks.

    Starting an empty task tree is no-op and the relevant
    warning message is issued.

    \sa setRecipe(), start()
*/
QTaskTree::QTaskTree(QObject *parent)
    : QObject(*new QTaskTreePrivate(this), parent)
{}

/*!
    \overload

    Constructs a task tree with a given \a recipe and \a parent.
    After the task tree is started, it executes the tasks contained
    inside the \a recipe and handles finished tasks according
    to the passed description.

    \sa setRecipe(), start()
*/
QTaskTree::QTaskTree(const Group &recipe, QObject *parent)
    : QTaskTree(parent)
{
    setRecipe(recipe);
}

/*!
    Destroys the task tree.

    When the task tree is running while being destructed,
    it cancels all the running tasks immediately. In this case,
    no handlers are called, not even the groups' and tasks' done handlers
    or onStorageDone() handlers. The task tree also doesn't emit any
    signals from the destructor, not even done() or progressValueChanged()
    signals. This behavior may always be relied on.
    It is completely safe to destruct the running task tree.

    It's a usual pattern to destruct the running task tree.
    It's guaranteed that the destruction will run quickly,
    without having to wait for the currently running tasks to finish,
    provided that the used tasks implement their destructors in a
    non-blocking way.

    \note Do not call the destructor directly from any of the running
          task's handlers or task tree's signals. In these cases,
          use \l deleteLater() instead.

    \sa cancel()
*/
QTaskTree::~QTaskTree()
{
    Q_D(QTaskTree);
    QT_TASKTREE_ASSERT(!d->m_guard.isLocked(), qWarning("Deleting QTaskTree instance directly from "
              "one of its handlers will lead to a crash!"));
    // TODO: delete storages explicitly here?
}

/*!
    Sets a given \a recipe for the task tree. After the task tree is started,
    it executes the tasks contained inside the \a recipe and
    handles finished tasks according to the passed description.

    \note When called for a running task tree, the call is ignored.

    \sa QTaskTree(const QtTaskTree::Group &recipe, QObject *parent = nullptr), start()
*/
void QTaskTree::setRecipe(const Group &recipe)
{
    Q_D(QTaskTree);
    QT_TASKTREE_ASSERT(!isRunning(), qWarning("The QTaskTree is already running, ignoring..."); return);
    QT_TASKTREE_ASSERT(!d->m_guard.isLocked(), qWarning("The setRecipe() is called from one of the"
                                               "QTaskTree handlers, ignoring..."); return);
    // TODO: Should we clear the m_storageHandlers, too?
    d->m_storages.clear();
    d->m_root.emplace(d, recipe);
}

/*!
    Starts the task tree.

    Use setRecipe() or the constructor to set the declarative description
    according to which the task tree will execute the contained tasks
    and handle finished tasks.

    When the task tree is empty, that is, constructed with a
    default constructor, a call to start() is no-op and the relevant
    warning message is issued.

    Otherwise, when the task tree is already running, a call to start()
    is ignored and the relevant warning message is issued.

    Otherwise, the task tree is started.

    The started task tree may finish synchronously, for example when
    the main group's start handler returns
    \l {QtTaskTree::} {SetupResult::StopWithError}.
    For this reason, the connection to the done signal should be
    established before calling start(). Use isRunning() in order to detect
    whether the task tree is still running after a call to start().

    The task tree implementation relies on the running event loop.
    Make sure you have a QEventLoop, QCoreApplication, or one of its
    subclasses running (or about to be run) when calling this method.

    \sa QTaskTree(const QtTaskTree::Group &recipe, QObject *parent = nullptr), setRecipe(), isRunning(), cancel()
*/
void QTaskTree::start()
{
    Q_D(QTaskTree);
    QT_TASKTREE_ASSERT(!isRunning(), qWarning("The QTaskTree is already running, ignoring..."); return);
    QT_TASKTREE_ASSERT(!d->m_guard.isLocked(), qWarning("The start() is called from one of the"
                                               "QTaskTree handlers, ignoring..."); return);
    d->start();
}

/*!
    \fn void QTaskTree::started()

    This signal is emitted when the task tree is started.
    The emission of this signal is followed synchronously by the
    progressValueChanged() signal with an initial \c 0 value.

    \sa start(), done()
*/

/*!
    \fn void QTaskTree::done(QtTaskTree::DoneWith result)

    This signal is emitted when the task tree finished, passing the
    final \a result of the execution. The task tree neither calls any handler,
    nor emits any signal anymore after this signal was emitted.

    \note Do not delete the task tree directly from this signal's handler.
          Use deleteLater() instead.

    \sa started()
*/

/*!
    Cancels the execution of the running task tree.

    Cancels all running tasks immediately. All running tasks and groups
    finish with an error, invoking their done handlers with
    \l {QtTaskTree::} {DoneWith::Cancel}.
    The storages' onStorageDone() handlers are invoked, too.
    The progressValueChanged() signals are also being sent.
    This behavior may always be relied on.

    The cancel() function is executed synchronously, so that after a call to
    cancel() all running tasks are finished and the tree is already canceled.
    It's guaranteed that cancel() will run quickly, without any blocking
    wait for the currently running tasks to finish, provided the used tasks
    implement their destructors in a non-blocking way.

    When the task tree is empty, that is, constructed with a default
    constructor, a call to cancel() is no-op and the relevant
    warning message is issued.

    Otherwise, when the task tree wasn't started, a call to cancel()
    is ignored.

    \note Do not call this function directly from any of the running
          task's handlers or task tree's signals.

    \sa ~QTaskTree()
*/
void QTaskTree::cancel()
{
    Q_D(QTaskTree);
    QT_TASKTREE_ASSERT(!d->m_guard.isLocked(), qWarning("The cancel() is called from one of the"
                                               "QTaskTree handlers, ignoring..."); return);
    d->stop();
}

/*!
    Returns \c true if the task tree is currently running;
    otherwise returns \c false.

    \sa start(), cancel()
*/
bool QTaskTree::isRunning() const
{
    return bool(d_func()->m_runtimeRoot);
}

/*!
    Executes a local event loop with QEventLoop::ExcludeUserInputEvents
    and starts the task tree.

    Returns \l {QtTaskTree::} {DoneWith::Success}
    if the task tree finished successfully; otherwise returns
    \l {QtTaskTree::} {DoneWith::Error}.

    \note Avoid using this method from the main thread.
          Use asynchronous start() instead.
          This method is to be used in non-main threads or in auto tests.

    \sa start()
*/
DoneWith QTaskTree::runBlocking()
{
    DoneWith doneWith = DoneWith::Cancel;
    QEventLoop loop;
    connect(this, &QTaskTree::done, &loop, [&loop, &doneWith](DoneWith result) {
        doneWith = result;
        // Otherwise, the tasks from inside the running tree that were deleteLater()
        // will be leaked. Refer to the QObject::deleteLater() docs.
        QMetaObject::invokeMethod(&loop, [&loop] { loop.quit(); }, Qt::QueuedConnection);
    });

    QTimer::singleShot(0, this, &QTaskTree::start);

    loop.exec(QEventLoop::ExcludeUserInputEvents);
    return doneWith;
}

/*!
    \overload runBlocking()

    The passed \a future is used for listening to the cancel event.
    When the task tree is canceled, this method cancels the passed \a future.
*/
#if QT_CONFIG(future)
DoneWith QTaskTree::runBlocking(const QFuture<void> &future)
{
    if (future.isCanceled())
        return DoneWith::Cancel;

    DoneWith doneWith = DoneWith::Cancel;
    QEventLoop loop;
    connect(this, &QTaskTree::done, &loop, [&loop, &doneWith](DoneWith result) {
        doneWith = result;
        // Otherwise, the tasks from inside the running tree that were deleteLater()
        // will be leaked. Refer to the QObject::deleteLater() docs.
        QMetaObject::invokeMethod(&loop, [&loop] { loop.quit(); }, Qt::QueuedConnection);
    });
    QFutureWatcher<void> watcher;
    connect(&watcher, &QFutureWatcherBase::canceled, this, &QTaskTree::cancel);
    watcher.setFuture(future);

    QTimer::singleShot(0, this, &QTaskTree::start);

    loop.exec(QEventLoop::ExcludeUserInputEvents);
    if (doneWith == DoneWith::Cancel) {
        auto nonConstFuture = future;
        nonConstFuture.cancel();
    }
    return doneWith;
}
#endif // QT_CONFIG(future)

/*!
    Constructs a temporary task tree using the passed \a recipe and
    runs it blocking.

    Returns \l {QtTaskTree::} {DoneWith::Success}
    if the task tree finished successfully; otherwise returns
    \l {QtTaskTree::} {DoneWith::Error}.

    \note Avoid using this method from the main thread.
          Use asynchronous start() instead.
          This method is to be used in non-main threads or in auto tests.

    \sa start()
*/
DoneWith QTaskTree::runBlocking(const Group &recipe)
{
    QTaskTree taskTree(recipe);
    return taskTree.runBlocking();
}

/*!
    \overload runBlocking(const Group &recipe)

    The passed \a future is used for listening to the cancel event.
    When the task tree is canceled, this method cancels the passed \a future.
*/
#if QT_CONFIG(future)
DoneWith QTaskTree::runBlocking(const Group &recipe, const QFuture<void> &future)
{
    QTaskTree taskTree(recipe);
    return taskTree.runBlocking(future);
}
#endif // QT_CONFIG(future)

/*!
    Returns the current real count of asynchronous chains of invocations.

    The returned value indicates how many times the control returns
    to the caller's event loop while the task tree is running. Initially,
    this value is \c 0. If the execution of the task tree finishes
    fully synchronously, this value remains \c 0. If the task tree
    contains any asynchronous tasks that are successfully started during
    a call to start(), this value is bumped to \c 1 just before the call
    to start() finishes. Later, when any asynchronous task finishes
    and any possible continuations are started, this value is bumped again.
    The bumping continues until the task tree finishes.
    When the task tree emits the done() signal, the bumping stops.
    The asyncCountChanged() signal is emitted on every bump of this value.

    \sa asyncCountChanged()
*/
int QTaskTree::asyncCount() const
{
    return d_func()->m_asyncCount;
}

/*!
    \fn void QTaskTree::asyncCountChanged(int count)

    This signal is emitted when the running task tree is about to
    return control to the caller's event loop. When the task tree is started,
    this signal is emitted with \a count value of \c 0, and emitted later
    on every asyncCount() value bump with an updated \a count value.
    Every signal sent (except the initial one with the value of \c 0)
    guarantees that the task tree is still running asynchronously
    after the emission.

    \sa asyncCount()
*/

/*!
    Returns the number of asynchronous tasks contained in the stored recipe.

    \note The returned number doesn't include \l QSyncTask tasks.
    \note Any task or group that was set up using withTimeout() increases
          the total number of tasks by \c 1.

    \sa setRecipe(), progressMaximum()
*/
int QTaskTree::taskCount() const
{
    Q_D(const QTaskTree);
    return d->m_root ? d->m_root->taskCount() : 0;
}

/*!
    \fn void QTaskTree::progressValueChanged(int value)

    This signal is emitted when the running task tree finished, canceled,
    or skipped some tasks. The \a value gives the current total number of
    finished, canceled or skipped tasks. When the task tree is started,
    and after the started() signal was emitted, this signal is emitted
    with an initial \a value of \c 0. When the task tree is about to finish,
    and before the done() signal is emitted,
    this signal is emitted with the final \a value of progressMaximum().

    \sa progressValue(), progressMaximum()
*/

/*!
    \fn int QTaskTree::progressMaximum() const

    Returns the maximum progressValue().

    \note Currently, it's the same as taskCount().
          This might change in the future.

    \sa progressValue()
*/

/*!
    Returns the current progress value, which is between the
    \c 0 and progressMaximum().

    The returned number indicates how many tasks have been already finished,
    canceled, or skipped while the task tree is running.
    When the task tree is started, this number is set to \c 0.
    When the task tree is finished, this number always equals progressMaximum().

    \sa progressMaximum(), progressValueChanged()
*/
int QTaskTree::progressValue() const
{
    return d_func()->m_progressValue;
}

/*!
    \fn template <typename StorageStruct, typename Handler> void QTaskTree::onStorageSetup(const QtTaskTree::Storage<StorageStruct> &storage, Handler &&handler)

    Installs a storage setup \a handler for the \a storage to pass
    the initial data dynamically to the running task tree.

    The \c StorageHandler takes a \e reference to the \c StorageStruct instance:

    \code
        static void save(const QString &fileName, const QByteArray &array) { ... }

        Storage<QByteArray> storage;

        const auto onSaverSetup = [storage](QThreadFunction<QByteArray> &task) {
            task.setThreadFunctionData(&save, "foo.txt", *storage);
        };

        const Group root {
            storage,
            QThreadFunctionTask(onSaverSetup)
        };

        QTaskTree taskTree(root);
        auto initStorage = [](QByteArray &storage){
            storage = "initial content";
        };
        taskTree.onStorageSetup(storage, initStorage);
        taskTree.start();
    \endcode

    When the running task tree enters a Group where the \a storage is
    placed in, it creates a \c StorageStruct instance, ready to be used
    inside this group. Just after the \c StorageStruct instance is created,
    and before any handler of this group is called, the task tree invokes
    the passed \a handler. This enables setting up initial content for
    the given storage dynamically. Later, when any group's handler is invoked,
    the task tree activates the created and initialized storage,
    so that it's available inside any group's handler.

    \sa onStorageDone()
*/

/*!
    \fn template <typename StorageStruct, typename Handler> void QTaskTree::onStorageDone(const QtTaskTree::Storage<StorageStruct> &storage, Handler &&handler)

    Installs a storage done \a handler for the \a storage to retrieve
    the final data dynamically from the running task tree.

    The \c StorageHandler takes a \c const \e reference to
    the \c StorageStruct instance:

    \code
        static QByteArray load(const QString &fileName) { ... }

        Storage<QByteArray> storage;

        const auto onLoaderSetup = [](QThreadFunction<QByteArray> &task) {
            task.setThreadFunctionData(&load, "foo.txt");
        };
        const auto onLoaderDone = [storage](const QThreadFunction<QByteArray> &task) {
            *storage = task.result();
        };

        const Group root {
            storage,
            QThreadFunctionTask(onLoaderSetup, onLoaderDone, CallDone::OnSuccess)
        };

        QTaskTree taskTree(root);
        auto collectStorage = [](const QByteArray &storage){
            qDebug() << "final content" << storage;
        };
        taskTree.onStorageDone(storage, collectStorage);
        taskTree.start();
    \endcode

    When the running task tree is about to leave a
    \l {QtTaskTree::} {Group} where the \a storage is placed in,
    it destructs a \c StorageStruct instance.
    Just before the \c StorageStruct instance is destructed, and after
    all possible handlers from this group were called, the task tree
    invokes the passed \a handler. This enables reading the final content
    of the given storage dynamically and processing it further outside of
    the task tree.

    This handler is called also when the running tree is canceled.
    However, it's not called when the running tree is destructed.

    \sa onStorageSetup()
*/

void QTaskTree::setupStorageHandler(const StorageBase &storage,
                                    const StorageBase::StorageHandler &setupHandler,
                                    const StorageBase::StorageHandler &doneHandler)
{
    Q_D(QTaskTree);
    auto it = d->m_storageHandlers.find(storage);
    if (it == d->m_storageHandlers.end()) {
        d->m_storageHandlers.insert(storage, {setupHandler, doneHandler});
        return;
    }
    if (setupHandler) {
        QT_TASKTREE_ASSERT(!it->m_setupHandler,
                  qWarning("The storage has its setup handler defined, overriding..."));
        it->m_setupHandler = setupHandler;
    }
    if (doneHandler) {
        QT_TASKTREE_ASSERT(!it->m_doneHandler,
                  qWarning("The storage has its done handler defined, overriding..."));
        it->m_doneHandler = doneHandler;
    }
}

void QTaskTreeTaskAdapter::operator()(QTaskTree *task, QTaskInterface *iface)
{
    QObject::connect(task, &QTaskTree::done, iface, [iface](DoneWith result) {
        iface->reportDone(toDoneResult(result));
    });
    task->start();
}

using TimeoutCallback = std::function<void()>;

struct TimerData
{
    size_t m_timerId;
    system_clock::time_point m_deadline;
    QPointer<QObject> m_context;
    TimeoutCallback m_callback;
};

static bool deadlineComp(const TimerData &a, const TimerData &b)
{
    return a.m_deadline < b.m_deadline;
}

struct TimerThreadData
{
    Q_DISABLE_COPY_MOVE(TimerThreadData)

    TimerThreadData() = default; // defult constructor is required for initializing with {} since C++20 by Mingw 11.20
    QList<TimerData> m_timerDataList = {};
    size_t m_timerIdCounter = 0;
};

// Please note the thread_local keyword below guarantees a separate instance per thread.
static thread_local TimerThreadData s_threadTimerData = {};

static void removeTimerId(size_t timerId)
{
    const auto it = std::find_if(s_threadTimerData.m_timerDataList.cbegin(),
                                 s_threadTimerData.m_timerDataList.cend(),
                                 [timerId](const TimerData &timerData) {
        return timerId == timerData.m_timerId;
    });
    QT_TASKTREE_ASSERT(it != s_threadTimerData.m_timerDataList.cend(),
                       qWarning("Removing active timerId failed."); return);

    s_threadTimerData.m_timerDataList.erase(it);
}

static void handleTimeout(const system_clock::time_point &deadline)
{
    while (true) {
        const auto it = s_threadTimerData.m_timerDataList.cbegin();
        if (it == s_threadTimerData.m_timerDataList.cend() || it->m_deadline > deadline)
            break;
        TimeoutCallback callback;
        if (it->m_context)
            std::exchange(callback, it->m_callback);
        s_threadTimerData.m_timerDataList.erase(it);
        if (callback)
            callback(); // Note: calling callback may invalidate m_timerDataList iterators.
    }
}

static size_t scheduleTimeout(milliseconds timeout, QObject *context, const TimeoutCallback &callback)
{
    const auto timerId = ++s_threadTimerData.m_timerIdCounter;
    const system_clock::time_point deadline = system_clock::now() + timeout;
    QTimer::singleShot(timeout, context, [deadline] { handleTimeout(deadline); });
    TimerData timerData{timerId, deadline, context, callback};
    const auto it = std::upper_bound(s_threadTimerData.m_timerDataList.cbegin(),
                                     s_threadTimerData.m_timerDataList.cend(),
                                     timerData, &deadlineComp);
    s_threadTimerData.m_timerDataList.insert(it, std::move(timerData));
    return timerId;
}

QTimeoutTaskAdapter::~QTimeoutTaskAdapter()
{
    if (m_timerId)
        removeTimerId(*m_timerId);
}

void QTimeoutTaskAdapter::operator()(milliseconds *task, QTaskInterface *iface)
{
    m_timerId = scheduleTimeout(*task, iface, [this, iface] {
        m_timerId.reset();
        iface->reportDone(DoneResult::Success);
    });
}

/*!
    \typedef QTaskTreeTask
    \relates QCustomTask

    Type alias for the QCustomTask<QTaskTree>, to be used inside recipes.
*/

/*!
    \typedef QTimeoutTask
    \relates QCustomTask

    Type alias for the QCustomTask<\c std::chrono::milliseconds>,
    to be used inside recipes. The \c std::chrono::milliseconds
    is used to set up the timeout duration. The default timeout is
    \c std::chrono::milliseconds::zero(), that is, the QTimeoutTask
    finishes as soon as the control returns to the running event loop.

    Example usage:

    \code
        using namespace std::chrono;
        using namespace std::chrono_literals;

        const auto onSetup = [](milliseconds &timeout) { timeout = 1000ms; }
        const auto onDone = [] { qDebug() << "Timed out."; }

        const Group root {
            QTimeoutTask(onSetup, onDone)
        };
    \endcode

    \sa {QtTaskTree::timeoutTask()} {timeoutTask()}
*/

QT_END_NAMESPACE
