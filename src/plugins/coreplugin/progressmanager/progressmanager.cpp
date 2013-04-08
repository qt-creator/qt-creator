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

#include "futureprogress.h"
#include "progressmanager_p.h"
#include "progressbar.h"
#include "progressview.h"
#include "../actionmanager/actionmanager.h"
#include "../icontext.h"
#include "../coreconstants.h"
#include "../icore.h"
#include "../statusbarwidget.h"


#include <extensionsystem/pluginmanager.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>

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
        \li Property
        \li Type
        \li Description
    \row
        \li Task abstraction
        \li \c QFuture<void>
        \li A \c QFuture object that represents the task which is
           responsible for reporting the state of the task. See below
           for coding patterns how to create this object for your
           specific task.
    \row
        \li Title
        \li \c QString
        \li A very short title describing your task. This is shown
           as a title over the progress bar.
    \row
        \li Type
        \li \c QString
        \li A string identifier that is used to group different tasks that
           belong together.
           For example, all the search operations use the same type
           identifier.
    \row
        \li Flags
        \li \l ProgressManager::ProgressFlags
        \li Additional flags that specify how the progress bar should
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
    m_applicationTask(0),
    m_currentStatusDetailsWidget(0),
    m_opacityEffect(new QGraphicsOpacityEffect(this)),
    m_progressViewPinned(false),
    m_hovered(false)
{
    m_progressView = new ProgressView;
    // withDelay, so the statusBarWidget has the chance to get the enter event
    connect(m_progressView, SIGNAL(hoveredChanged(bool)), this, SLOT(updateVisibilityWithDelay()));
    connect(ICore::instance(), SIGNAL(coreAboutToClose()), this, SLOT(cancelAllRunningTasks()));
}

ProgressManagerPrivate::~ProgressManagerPrivate()
{
    qDeleteAll(m_taskList);
    m_taskList.clear();
    ExtensionSystem::PluginManager::removeObject(m_statusBarWidgetContainer);
    delete m_statusBarWidgetContainer;
    cleanup();
}

void ProgressManagerPrivate::init()
{
    m_statusBarWidgetContainer = new Core::StatusBarWidget;
    m_statusBarWidget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(m_statusBarWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_statusBarWidget->setLayout(layout);
    m_summaryProgressWidget = new QWidget(m_statusBarWidget);
    m_summaryProgressWidget->setVisible(!m_progressViewPinned);
    m_summaryProgressWidget->setGraphicsEffect(m_opacityEffect);
    m_summaryProgressLayout = new QHBoxLayout(m_summaryProgressWidget);
    m_summaryProgressLayout->setContentsMargins(0, 0, 0, 0);
    m_summaryProgressLayout->setSpacing(0);
    m_summaryProgressWidget->setLayout(m_summaryProgressLayout);
    m_summaryProgressBar = new ProgressBar(m_summaryProgressWidget);
    m_summaryProgressBar->setMinimumWidth(70);
    m_summaryProgressBar->setTitleVisible(false);
    m_summaryProgressBar->setSeparatorVisible(false);
    m_summaryProgressBar->setCancelEnabled(false);
    m_summaryProgressLayout->addWidget(m_summaryProgressBar);
    layout->addWidget(m_summaryProgressWidget);
    ToggleButton *toggleButton = new ToggleButton(m_statusBarWidget);
    layout->addWidget(toggleButton);
    m_statusBarWidgetContainer->setWidget(m_statusBarWidget);
    m_statusBarWidgetContainer->setPosition(Core::StatusBarWidget::RightCorner);
    ExtensionSystem::PluginManager::addObject(m_statusBarWidgetContainer);
    m_statusBarWidget->installEventFilter(this);

    QAction *toggleProgressView = new QAction(tr("Toggle progress details"), this);
    toggleProgressView->setCheckable(true);
    toggleProgressView->setChecked(m_progressViewPinned);
    // we have to set an transparent icon to prevent the tool button to show text
    QPixmap p(1, 1);
    p.fill(Qt::transparent);
    toggleProgressView->setIcon(QIcon(p));
    Command *cmd = ActionManager::registerAction(toggleProgressView,
                                                 Id("QtCreator.ToggleProgressDetails"),
                                                 Context(Constants::C_GLOBAL));
    cmd->setDefaultKeySequence(QKeySequence(Utils::HostOsInfo::isMacHost()
                                               ? tr("Ctrl+Shift+0")
                                               : tr("Alt+Shift+0")));
    connect(toggleProgressView, SIGNAL(toggled(bool)), this, SLOT(progressDetailsToggled(bool)));
    toggleButton->setDefaultAction(cmd->action());

    m_progressView->setVisible(m_progressViewPinned);

    initInternal();
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
    if (found) {
        updateSummaryProgressBar();
        emit allTasksFinished(type);
    }
}

bool ProgressManagerPrivate::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_statusBarWidget && event->type() == QEvent::Enter) {
        m_hovered = true;
        updateVisibility();
    } else if (obj == m_statusBarWidget && event->type() == QEvent::Leave) {
        m_hovered = false;
        // give the progress view the chance to get the mouse enter event
        updateVisibilityWithDelay();
    }
    return false;
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
    updateSummaryProgressBar();
}

FutureProgress *ProgressManagerPrivate::addTask(const QFuture<void> &future, const QString &title,
                                                const QString &type, ProgressFlags flags)
{
    // watch
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>();
    m_runningTasks.insert(watcher, type);
    connect(watcher, SIGNAL(progressRangeChanged(int,int)), this, SLOT(updateSummaryProgressBar()));
    connect(watcher, SIGNAL(progressValueChanged(int)), this, SLOT(updateSummaryProgressBar()));
    connect(watcher, SIGNAL(finished()), this, SLOT(taskFinished()));
    watcher->setFuture(future);

    // handle application task
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

    // create FutureProgress and manage task list
    removeOldTasks(type);
    if (m_taskList.size() == 10)
        removeOneOldTask();
    FutureProgress *progress = new FutureProgress;
    progress->setTitle(title);
    progress->setFuture(future);

    m_progressView->addProgressWidget(progress);
    m_taskList.append(progress);
    progress->setType(type);
    if (flags.testFlag(ProgressManager::KeepOnFinish))
        progress->setKeepOnFinish(FutureProgress::KeepOnFinishTillUserInteraction);
    else
        progress->setKeepOnFinish(FutureProgress::HideOnFinish);
    connect(progress, SIGNAL(hasErrorChanged()), this, SLOT(updateSummaryProgressBar()));
    connect(progress, SIGNAL(removeMe()), this, SLOT(slotRemoveTask()));
    connect(progress, SIGNAL(fadeStarted()), this, SLOT(updateSummaryProgressBar()));
    connect(progress, SIGNAL(statusBarWidgetChanged()), this, SLOT(updateStatusDetailsWidget()));
    updateStatusDetailsWidget();

    emit taskStarted(type);
    return progress;
}

ProgressView *ProgressManagerPrivate::progressView()
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
    updateSummaryProgressBar();

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

void ProgressManagerPrivate::updateSummaryProgressBar()
{
    m_summaryProgressBar->setError(hasError());
    updateVisibility();
    if (m_runningTasks.isEmpty()) {
        m_summaryProgressBar->setFinished(true);
        if (m_taskList.isEmpty() || isLastFading())
            fadeAwaySummaryProgress();
        return;
    }

    stopFadeOfSummaryProgress();

    m_summaryProgressBar->setFinished(false);
    QMapIterator<QFutureWatcher<void> *, QString> it(m_runningTasks);
    int range = 0;
    int value = 0;
    while (it.hasNext()) {
        it.next();
        QFutureWatcher<void> *watcher = it.key();
        int min = watcher->progressMinimum();
        range += watcher->progressMaximum() - min;
        value += watcher->progressValue() - min;
    }
    m_summaryProgressBar->setRange(0, range);
    m_summaryProgressBar->setValue(value);
}

void ProgressManagerPrivate::fadeAwaySummaryProgress()
{
    stopFadeOfSummaryProgress();
    m_opacityAnimation = new QPropertyAnimation(m_opacityEffect, "opacity");
    m_opacityAnimation->setDuration(Utils::StyleHelper::progressFadeAnimationDuration);
    m_opacityAnimation->setEndValue(0.);
    connect(m_opacityAnimation, SIGNAL(finished()), this, SLOT(summaryProgressFinishedFading()));
    m_opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ProgressManagerPrivate::stopFadeOfSummaryProgress()
{
    if (m_opacityAnimation) {
        m_opacityAnimation->stop();
        m_opacityEffect->setOpacity(1.);
        delete m_opacityAnimation;
    }
}

bool ProgressManagerPrivate::hasError() const
{
    foreach (FutureProgress *progress, m_taskList)
        if (progress->hasError())
            return true;
    return false;
}

bool ProgressManagerPrivate::isLastFading() const
{
    if (m_taskList.isEmpty())
        return false;
    foreach (FutureProgress *progress, m_taskList) {
        if (!progress->isFading()) // we still have progress bars that are not fading
            return false;
    }
    return true;
}

void ProgressManagerPrivate::slotRemoveTask()
{
    FutureProgress *progress = qobject_cast<FutureProgress *>(sender());
    QTC_ASSERT(progress, return);
    QString type = progress->type();
    removeTask(progress);
    removeOldTasks(type, true);
}

void ProgressManagerPrivate::removeOldTasks(const QString &type, bool keepOne)
{
    bool firstFound = !keepOne; // start with false if we want to keep one
    QList<FutureProgress *>::iterator i = m_taskList.end();
    while (i != m_taskList.begin()) {
        --i;
        if ((*i)->type() == type) {
            if (firstFound && ((*i)->future().isFinished() || (*i)->future().isCanceled())) {
                deleteTask(*i);
                i = m_taskList.erase(i);
            }
            firstFound = true;
        }
    }
    updateSummaryProgressBar();
    updateStatusDetailsWidget();
}

void ProgressManagerPrivate::removeOneOldTask()
{
    if (m_taskList.isEmpty())
        return;
    // look for oldest ended process
    for (QList<FutureProgress *>::iterator i = m_taskList.begin(); i != m_taskList.end(); ++i) {
        if ((*i)->future().isFinished()) {
            deleteTask(*i);
            i = m_taskList.erase(i);
            return;
        }
    }
    // no ended process, look for a task type with multiple running tasks and remove the oldest one
    for (QList<FutureProgress *>::iterator i = m_taskList.begin(); i != m_taskList.end(); ++i) {
        QString type = (*i)->type();

        int taskCount = 0;
        foreach (FutureProgress *p, m_taskList)
            if (p->type() == type)
                ++taskCount;

        if (taskCount > 1) { // don't care for optimizations it's only a handful of entries
            deleteTask(*i);
            i = m_taskList.erase(i);
            return;
        }
    }

    // no ended process, no type with multiple processes, just remove the oldest task
    FutureProgress *task = m_taskList.takeFirst();
    deleteTask(task);
    updateSummaryProgressBar();
    updateStatusDetailsWidget();
}

void ProgressManagerPrivate::removeTask(FutureProgress *task)
{
    m_taskList.removeAll(task);
    deleteTask(task);
    updateSummaryProgressBar();
    updateStatusDetailsWidget();
}

void ProgressManagerPrivate::deleteTask(FutureProgress *progress)
{
    m_progressView->removeProgressWidget(progress);
    progress->hide();
    progress->deleteLater();
}

void ProgressManagerPrivate::updateVisibility()
{
    m_progressView->setVisible(m_progressViewPinned || m_hovered || m_progressView->isHovered());
    m_summaryProgressWidget->setVisible((!m_runningTasks.isEmpty() || !m_taskList.isEmpty())
                                     && !m_progressViewPinned);
}

void ProgressManagerPrivate::updateVisibilityWithDelay()
{
    QTimer::singleShot(150, this, SLOT(updateVisibility()));
}

void ProgressManagerPrivate::updateStatusDetailsWidget()
{
    QWidget *candidateWidget = 0;
    // get newest progress with a status bar widget
    QList<FutureProgress *>::iterator i = m_taskList.end();
    while (i != m_taskList.begin()) {
        --i;
        candidateWidget = (*i)->statusBarWidget();
        if (candidateWidget)
            break;
    }

    if (candidateWidget == m_currentStatusDetailsWidget)
        return;

    if (m_currentStatusDetailsWidget) {
        m_currentStatusDetailsWidget->hide();
        m_summaryProgressLayout->removeWidget(m_currentStatusDetailsWidget);
    }

    if (candidateWidget) {
        m_summaryProgressLayout->insertWidget(0, candidateWidget);
        candidateWidget->show();
    }

    m_currentStatusDetailsWidget = candidateWidget;
}

void ProgressManagerPrivate::summaryProgressFinishedFading()
{
    m_summaryProgressWidget->setVisible(false);
    m_opacityEffect->setOpacity(1.);
}

void ProgressManagerPrivate::progressDetailsToggled(bool checked)
{
    m_progressViewPinned = checked;
    updateVisibility();
}

ToggleButton::ToggleButton(QWidget *parent)
    : QToolButton(parent)
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
}

QSize ToggleButton::sizeHint() const
{
    return QSize(12, 12);
}

void ToggleButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event);
    QPainter p(this);
    QStyle *s = style();
    QStyleOption arrowOpt;
    arrowOpt.initFrom(this);
    arrowOpt.rect = QRect(rect().center().x() - 3, rect().center().y() - 6, 9, 9);
    s->drawPrimitive(QStyle::PE_IndicatorArrowUp, &arrowOpt, &p, this);
}
