/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "progressmanager_p.h"
#include "progressview.h"
#include "../coreconstants.h"
#include "../icore.h"

#include <utils/qtcassert.h>

using namespace Core;
using namespace Core::Internal;

/*!
    \mainclass
    \class Core::ProgressManager
    \brief The ProgressManager class is used to show a user interface
    for running tasks in Qt Creator.

    It tracks the progress of a task that it is told
    about, and shows a progress indicator in the left hand tool bar
    of Qt Creator's main window to the user.
    The progress indicator also allows the user to cancel the task.

    You get the single instance of this class via the
    Core::ICore::progressManager() method.

    \section1 Registering a task
    The ProgressManager API uses QtConcurrent as the basis for defining
    tasks. A task consists of the following properties:

    \table
    \header
        \o Property
        \o Type
        \o Description
    \row
        \o Task abstraction
        \o \c QFuture<void>
        \o A \c QFuture object that represents the task which is
           responsible for reporting the state of the task. See below
           for coding patterns how to create this object for your
           specific task.
    \row
        \o Title
        \o \c QString
        \o A very short title describing your task. This is shown
           as a title over the progress bar.
    \row
        \o Type
        \o \c QString
        \o A string identifier that is used to group different tasks that
           belong together.
           For example, all the search operations use the same type
           identifier.
    \row
        \o Flags
        \o \l ProgressManager::ProgressFlags
        \o Additional flags that specify how the progress bar should
           be presented to the user.
    \endtable

    To register a task you create your \c QFuture<void> object, and call
    addTask(). This method returns a
    \l{Core::FutureProgress}{FutureProgress}
    object that you can use to further customize the progress bar's appearance.
    See the \l{Core::FutureProgress}{FutureProgress} documentation for
    details.

    In the following you will learn about two common patterns how to
    create the \c QFuture<void> object for your task.

    \section2 Create a threaded task with QtConcurrent
    The first option is to directly use QtConcurrent to actually
    start a task concurrently in a different thread.
    QtConcurrent has several different methods to run e.g.
    a class method in a different thread. Qt Creator itself
    adds a few more in \c{src/libs/qtconcurrent/runextensions.h}.
    The QtConcurrent methods to run a concurrent task return a
    \c QFuture object. This is what you want to give the
    ProgressManager in the addTask() method.

    Have a look at e.g Locator::ILocatorFilter. Locator filters implement
    a method \c refresh which takes a \c QFutureInterface object
    as a parameter. These methods look something like:
    \code
    void Filter::refresh(QFutureInterface<void> &future) {
        future.setProgressRange(0, MAX);
        ...
        while (!future.isCanceled()) {
            // Do a part of the long stuff
            ...
            future.setProgressValue(currentProgress);
            ...
        }
    }
    \endcode

    The actual refresh, which calls all the filters' refresh methods
    in a different thread, looks like this:
    \code
    QFuture<void> task = QtConcurrent::run(&ILocatorFilter::refresh, filters);
    Core::FutureProgress *progress = Core::ICore::instance()
            ->progressManager()->addTask(task, tr("Indexing"),
                                         Locator::Constants::TASK_INDEX);
    \endcode
    First, we tell QtConcurrent to start a thread which calls all the filters'
    refresh method. After that we register the returned QFuture object
    with the ProgressManager.

    \section2 Manually create QtConcurrent objects for your thread
    If your task has its own means to create and run a thread,
    you need to create the necessary objects yourselves, and
    report the start/stop state.

    \code
    // We are already running in a different thread here
    QFutureInterface<void> *progressObject = new QFutureInterface<void>;
    progressObject->setProgressRange(0, MAX);
    Core::ICore::progressManager()->addTask(
        progressObject->future(),
        tr("DoIt"), MYTASKTYPE);
    progressObject->reportStarted();
    // Do something
    ...
    progressObject->setProgressValue(currentProgress);
    ...
    // We have done what we needed to do
    progressObject->reportFinished();
    delete progressObject;
    \endcode
    In the first line we create the QFutureInterface object that will be
    our way for reporting the task's state.
    The first thing we report is the expected range of the progress values.
    We register the task with the ProgressManager, using the internal
    QFuture object that has been created for our QFutureInterface object.
    Next we report that the task has begun and start doing our actual
    work, regularly reporting the progress via the methods
    in QFutureInterface. After the long taking operation has finished,
    we report so through the QFutureInterface object, and delete it
    afterwards.

    \section1 Customizing progress appearance

    You can set a custom widget to show below the progress bar itself,
    using the FutureProgress object returned by the addTask() method.
    Also use this object to get notified when the user clicks on the
    progress indicator.
*/

/*!
    \enum Core::ProgressManager::ProgressFlag
    Additional flags that specify details in behavior. The
    default for a task is to not have any of these flags set.
    \value KeepOnFinish
        The progress indicator stays visible after the task has finished.
    \value ShowInApplicationIcon
        The progress indicator for this task is additionally
        shown in the application icon in the system's task bar or dock, on
        platforms that support that (at the moment Windows 7 and Mac OS X).
*/

/*!
    \fn Core::ProgressManager::ProgressManager(QObject *parent = 0)
    \internal
*/

/*!
    \fn Core::ProgressManager::~ProgressManager()
    \internal
*/

/*!
    \fn FutureProgress *Core::ProgressManager::addTask(const QFuture<void> &future, const QString &title, const QString &type, ProgressFlags flags = 0)

    Shows a progress indicator for task given by the QFuture object \a future.
    The progress indicator shows the specified \a title along with the progress bar.
    The \a type of a task will specify a logical grouping with other
    running tasks. Via the \a flags parameter you can e.g. let the
    progress indicator stay visible after the task has finished.
    Returns an object that represents the created progress indicator,
    which can be used to further customize. The FutureProgress object's
    life is managed by the ProgressManager and is guaranteed to live only until
    the next event loop cycle, or until the next call of addTask.
    If you want to use the returned FutureProgress later than directly after calling this method,
    you will need to use protective methods (like wrapping the returned object in QPointer and
    checking for 0 whenever you use it).
*/

/*!
    \fn void Core::ProgressManager::setApplicationLabel(const QString &text)

    Shows the given \a text in a platform dependent way in the application
    icon in the system's task bar or dock. This is used
    to show the number of build errors on Windows 7 and Mac OS X.
*/

/*!
    \fn void Core::ProgressManager::cancelTasks(const QString &type)

    Schedules a cancel for all running tasks of the given \a type.
    Please note that the cancel functionality depends on the
    running task to actually check the \c QFutureInterface::isCanceled
    property.
*/

/*!
    \fn void Core::ProgressManager::taskStarted(const QString &type)

    Sent whenever a task of a given \a type is started.
*/

/*!
    \fn void Core::ProgressManager::allTasksFinished(const QString &type)

    Sent when all tasks of a \a type have finished.
*/

ProgressManagerPrivate::ProgressManagerPrivate(QObject *parent)
  : ProgressManager(parent),
    m_applicationTask(0)
{
    m_progressView = new ProgressView;
    connect(ICore::instance(), SIGNAL(coreAboutToClose()), this, SLOT(cancelAllRunningTasks()));
}

ProgressManagerPrivate::~ProgressManagerPrivate()
{
    cleanup();
}

void ProgressManagerPrivate::cancelTasks(const QString &type)
{
    bool found = false;
    QMap<QFutureWatcher<void> *, QString>::iterator task = m_runningTasks.begin();
    while (task != m_runningTasks.end()) {
        if (task.value() != type) {
            ++task;
            continue;
        }
        found = true;
        disconnect(task.key(), SIGNAL(finished()), this, SLOT(taskFinished()));
        if (m_applicationTask == task.key())
            disconnectApplicationTask();
        task.key()->cancel();
        delete task.key();
        task = m_runningTasks.erase(task);
    }
    if (found)
        emit allTasksFinished(type);
}

void ProgressManagerPrivate::cancelAllRunningTasks()
{
    QMap<QFutureWatcher<void> *, QString>::const_iterator task = m_runningTasks.constBegin();
    while (task != m_runningTasks.constEnd()) {
        disconnect(task.key(), SIGNAL(finished()), this, SLOT(taskFinished()));
        if (m_applicationTask == task.key())
            disconnectApplicationTask();
        task.key()->cancel();
        delete task.key();
        ++task;
    }
    m_runningTasks.clear();
}

FutureProgress *ProgressManagerPrivate::addTask(const QFuture<void> &future, const QString &title,
                                                const QString &type, ProgressFlags flags)
{
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>();
    m_runningTasks.insert(watcher, type);
    connect(watcher, SIGNAL(finished()), this, SLOT(taskFinished()));
    if (flags & ShowInApplicationIcon) {
        if (m_applicationTask)
            disconnectApplicationTask();
        m_applicationTask = watcher;
        setApplicationProgressRange(future.progressMinimum(), future.progressMaximum());
        setApplicationProgressValue(future.progressValue());
        connect(m_applicationTask, SIGNAL(progressRangeChanged(int,int)),
                this, SLOT(setApplicationProgressRange(int,int)));
        connect(m_applicationTask, SIGNAL(progressValueChanged(int)),
                this, SLOT(setApplicationProgressValue(int)));
        setApplicationProgressVisible(true);
    }
    watcher->setFuture(future);
    emit taskStarted(type);
    return m_progressView->addTask(future, title, type, flags);
}

QWidget *ProgressManagerPrivate::progressView()
{
    return m_progressView;
}

void ProgressManagerPrivate::taskFinished()
{
    QObject *taskObject = sender();
    QTC_ASSERT(taskObject, return);
    QFutureWatcher<void> *task = static_cast<QFutureWatcher<void> *>(taskObject);
    if (m_applicationTask == task)
        disconnectApplicationTask();
    QString type = m_runningTasks.value(task);
    m_runningTasks.remove(task);
    delete task;

    if (!m_runningTasks.key(type, 0))
        emit allTasksFinished(type);
}

void ProgressManagerPrivate::disconnectApplicationTask()
{
    disconnect(m_applicationTask, SIGNAL(progressRangeChanged(int,int)),
            this, SLOT(setApplicationProgressRange(int,int)));
    disconnect(m_applicationTask, SIGNAL(progressValueChanged(int)),
            this, SLOT(setApplicationProgressValue(int)));
    setApplicationProgressVisible(false);
    m_applicationTask = 0;
}
