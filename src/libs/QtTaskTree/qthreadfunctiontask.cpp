// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTaskTree/qthreadfunctiontask.h>

#include <QtCore/private/qobject_p.h>
#include <QThread>

QT_BEGIN_NAMESPACE

static bool isMainThread()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    return QThread::isMainThread();
#else
    return QThread::currentThread() == qApp->thread();
#endif
}

namespace QtTaskTree {

class FutureSynchronizer final
{
public:
    FutureSynchronizer() = default;
    ~FutureSynchronizer()
    {
        cancelAll();
        waitForFinished();
    }

    void waitForFinished()
    {
        for (QFuture<void> &future : m_futures)
            future.waitForFinished();
        m_futures.clear();
    }

    void cancelAll()
    {
        for (QFuture<void> &future : m_futures)
            future.cancel();
        flushFinishedFutures();
    }

    void flushFinishedFutures()
    {
        QList<QFuture<void>> newFutures;
        for (const QFuture<void> &future : std::as_const(m_futures)) {
            if (!future.isFinished())
                newFutures.append(future);
        }
        m_futures = newFutures;
    }

    void addFuture(const QFuture<void> &future)
    {
        m_futures.append(future);
        flushFinishedFutures();
    }

private:
    QList<QFuture<void>> m_futures;
};

} // namespace QtTaskTree

Q_APPLICATION_STATIC(QtTaskTree::FutureSynchronizer, s_futureSynchronizer);

class QThreadFunctionBasePrivate : public QObjectPrivate
{
public:
    bool isUsingSynchronizer() const { return m_isAutoSync && isMainThread(); }

    QThreadPool *m_threadPool = nullptr;
    bool m_isAutoSync = true;
    bool m_isSyncSkipped = false;
    std::optional<QFuture<void>> m_future;
};

/*!
    \class QThreadFunctionBase
    \inheaderfile qthreadfunctiontask.h
    \inmodule TaskTree
    \brief A base class for QThreadFunction class template.
    \reentrant
*/

/*!
    \class QThreadFunction
    \inheaderfile qthreadfunctiontask.h
    \inmodule TaskTree
    \brief A class template controlling the execution of a function in a
           separate thread via QtConcurrent::run().
    \reentrant

    A QThreadFunction is a convenient class template that combines a stored
    function to be called later via QtConcurrent::run() with an internal
    QFutureWatcher monitoring the function execution. Use QThreadFunctionTask
    to execute the QThreadFunction inside the QTaskTree recipe.

    Set a function with its arguments to be executed in a separate thread
    using setThreadFunctionData() method.

    To report an error from inside the function running in a separate thread,
    place the \c {QPromise<ResultType> &promise} argument in that function
    as the first argument, and cancel the promise's future. Example:

    \code
        static void copyFile(QPromise<void> &promise, const QString &source,
                             const QString &destination)
        {
            const QFileInfo fi(source);
            if (!fi.exists()) {
                promise.future().cancel(); // The QThreadFunction finishes with DoneResult::Error
                return;
            }

            // Copy file...
        }

        QThreadFunction<void> task;
        task.setThreadFunctionData(copyFile, "source.txt", "dest.txt");
        task.start();
    \endcode

    Use setThreadPool() to execute a function in a desired QThreadPool,
    otherwise the function will use the \l {QThreadPool::globalInstance()}
    {global thread pool}.

    Use setAutoDelayedSync() to switch the automatic delayed synchronization.

    The function execution can be controlled via started(), done(),
    resultReadyAt(), progressRangeChanged(), progressValueChanged(),
    or progressTextChanged() signals. The output data can be collected
    via result(), resultAt(), or results() getters.
*/

/*!
    \fn template <typename ResultType> QThreadFunction<ResultType>::QThreadFunction(QObject *parent = nullptr)

    Constructs a QThreadFunction with a given \a parent.
*/
QThreadFunctionBase::QThreadFunctionBase(QObject *parent)
    : QObject(*new QThreadFunctionBasePrivate, parent)
{}

/*!
    \fn template <typename ResultType> QThreadFunction<ResultType>::~QThreadFunction() override

    Destroys the QThreadFunction.
    If the QThreadFunction is not running, no other actions are performed,
    otherwise the associated future is canceled, and depending on
    the automatic delayed synchronization the following actions are performed:

    \table
    \header
        \li Automatic Delayed Synchronization
        \li Action
    \row
        \li On
        \li The associated QFuture is stored in the global registry.
            Use \l QThreadFunctionBase::syncAll() on application quit
            to synchronize all futures stored previously in the
            global registry.
    \row
        \li Off
        \li A blocking call to QFuture::waitForFinished() is executed
            directly.
    \endtable

    When the automatic delayed synchronization is on, the cancellation
    of the future may be intercepted inside the still running function
    in a separate thread, so that it may finish as soon as possible
    without completing its job.

    Finally, on application quit, \l QThreadFunctionBase::syncAll()
    should be called in order to synchronize all the functions still
    being potentially running in separate threads.
    The call to \l QThreadFunctionBase::syncAll() is blocking in
    case some functions are still finalizing.

    \note When the automatic delayed synchronization is on,
          the function will still run for a while after the QThreadFunction's
          destructor is finished, so it's the user responsibility to ensure
          that all data the function may operate on is still available.
          If that can't be guaranteed, set the automatic delayed
          synchronization to off via
          QThreadFunction::setAutoDelayedSync(false). In this case the
          QThreadFunction destructor will blocking wait for the function
          to be finished before deleting the QThreadFunction.
          Refer to QCustomTask documentation for more information
          about \c Deleter template parameter.
    \sa setAutoDelayedSync(), syncAll()
*/
QThreadFunctionBase::~QThreadFunctionBase()
{
    Q_D(QThreadFunctionBase);
    if (!d->m_future || d->m_future->isFinished())
        return;

    d->m_future->cancel();
    if (isSyncSkipped())
        return;

    if (d->isUsingSynchronizer())
        return;

    d->m_future->waitForFinished();
}

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::setThreadPool(QThreadPool *pool)

    Sets the QThreadPool \a pool to be used when executing the call.
    If the passed \a pool is \c nullptr, the QThreadPool::globalInstance()
    is used.
*/
void QThreadFunctionBase::setThreadPool(QThreadPool *pool) { d_func()->m_threadPool = pool; }

/*!
    \fn template <typename ResultType> QThreadPool *QThreadFunction<ResultType>::threadPool() const

    Returns the thread pool that is used when executing the call.
    If \c nullptr is returned, the QThreadPool::globalInstance() is used.
*/
QThreadPool *QThreadFunctionBase::threadPool() const { return d_func()->m_threadPool; }

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::setAutoDelayedSync(bool on)

    Sets the automatic delayed synchronization to \a on.

    By default, an automatic delayed synchronization is on, meaning
    the destruction of the running QThreadFunction object won't blocking
    wait until the function running in separate thread is finished.
    Instead, the associated QFuture is canceled,
    and the function is left running until it's finished.
    The cancellation can be intercepted by the function running
    in a separate thread via the QPromise argument, so that
    it may finish early without completing its job.
    In case the automatic delayed synchronization is on, it's necessary to call
    QThreadFunctionBase::syncAll() on application quit, in order
    to synchronize all the possibly running functions in separate threads.

    When the automatic delayed synchronization is off, the synchronization
    happens on QThreadFunction's destruction, what can block the caller
    thread for considerable amount of time.

    The automatic synchronization is used only when QThreadFunction
    was executed from the main thread.

    \sa syncAll(), isAutoDelayedSync()
*/
void QThreadFunctionBase::setAutoDelayedSync(bool on) { d_func()->m_isAutoSync = on; }

/*!
    \fn template <typename ResultType> bool QThreadFunction<ResultType>::isAutoDelayedSync() const

    Returns whether the automatic delayed synchronization is on.

    \sa syncAll(), setAutoDelayedSync()
*/
bool QThreadFunctionBase::isAutoDelayedSync() const { return d_func()->m_isAutoSync; }

/*!
    \internal

    Might be used for custom deleter performing custom synchronization
    on the associated QFuture. The default value is false.
*/
void QThreadFunctionBase::setSyncSkipped(bool on) { d_func()->m_isSyncSkipped = on; }

/*!
    \internal
*/
bool QThreadFunctionBase::isSyncSkipped() const { return d_func()->m_isSyncSkipped; }

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::started()

    This signal is emitted just after the execution of the stored function
    has started.

    \sa done()
*/

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::done(QtTaskTree::DoneResult result)

    This signal is emitted just after the execution of the stored function
    has finished. The passed \a result indicates whether the function
    finished with success or an error.

    \sa start(), isDone()
*/

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::resultReadyAt(int index)

    This signal is emitted whenever the function running in a separate thread
    reported any partial result at \a index via QPromise::addResult().
    To get the result, call resultAt(index);

    \sa resultAt(), QFutureWatcher::resultReadyAt()
*/

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::resultsReadyAt(int beginIndex, int endIndex)

    This signal is emitted whenever the function running in a separate thread
    reported any ready results indexed from \a beginIndex to \a endIndex.
    To get the result, call resultAt(index);

    \sa resultAt(), QFutureWatcher::resultsReadyAt()
*/

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::progressRangeChanged(int minimum, int maximum)

    This signal is emitted whenever the progress range of the
    concurrent call has changed to \a minimum and \a maximum.

    \sa QPromise::setProgressRange(), QFutureWatcher::progressRangeChanged()
*/

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::progressValueChanged(int value)

    This signal is emitted when the progress value of the
    function executed in a separate thread has changed
    to \a value.

    \sa QPromise::setProgressValue(), QFutureWatcher::progressValueChanged()
*/

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::progressTextChanged(const QString &text)

    This signal is emitted when the textual progress of the
    function executed in a separate thread has changed
    to \a text.

    \sa QPromise::setProgressValueAndText(), QFutureWatcher::progressTextChanged()
*/

/*!
    \fn template <typename ResultType> template <typename Function, typename ...Args> void QThreadFunction<ResultType>::setThreadFunctionData(Function &&function, Args &&...args)

    Sets the \a function to be executed with passed \a args in a separate
    thread when start() is called.

    \sa QtConcurrent::run()
*/

/*!
    \fn template <typename ResultType> bool QThreadFunction<ResultType>::isDone() const

    Returns whether the function execution is finished.

    \sa start(), done()
*/

/*!
    \fn template <typename ResultType> bool QThreadFunction<ResultType>::isResultAvailable() const

    Returns whether the result is ready.

    \sa result()
*/

/*!
    \fn template <typename ResultType> QFuture<ResultType> QThreadFunction<ResultType>::future() const

    Returns the QFuture<ResultType> associated with the function
    executed in a separate thread.
*/

/*!
    \fn template <typename ResultType> ResultType QThreadFunction<ResultType>::result() const

    Returns the ResultType reported by the function executed in a
    separate thread.

    \note Ensure the result is ready by calling isResultAvailable(),
          otherwise the call to result() may block in case the
          result wasn't reported yet, or even crash in case the
          function execution finished without reporting any result.

    \sa QFutureWatcher::result()
*/

/*!
    \fn template <typename ResultType> ResultType QThreadFunction<ResultType>::takeResult() const

    Takes (moves) the ResultType reported by the function executed in a
    separate thread.

    \note Ensure the result is ready by calling isResultAvailable(),
          otherwise the call to takeResult() may block in case the
          result wasn't reported yet, or even crash in case the
          function execution finished without reporting any result.

    \sa QFuture::takeResult()
*/

/*!
    \fn template <typename ResultType> ResultType QThreadFunction<ResultType>::resultAt(int index) const

    Returns the ResultType reported by the function executed in a
    separate thread at \a index.

    \sa result(), QFutureWatcher::resultAt()
*/

/*!
    \fn template <typename ResultType> QList<ResultType> QThreadFunction<ResultType>::results() const

    Returns the list of ResultType reported by the function executed in a
    separate thread.

    \sa result(), resultAt(), QFuture::results()
*/

/*!
    \fn template <typename ResultType> void QThreadFunction<ResultType>::start()

    Starts the asynchronous invocation of stored function in a separate
    thread. Connect to done() signal to collect the result reported
    by the function.

    \sa setThreadFunctionData(), done()
*/

/*!
    This method should be called on application quit to synchronize
    the finalization of all still possibly running functions that were
    started via QThreadFunction.
    The call has to be executed from the main thread.

    \sa QThreadFunction::setAutoDelayedSync()
*/
void QThreadFunctionBase::syncAll()
{
    if (!isMainThread()) {
        qWarning("The QThreadFunctionBase::syncAll() should be called from the main thread. "
                 "The current call is ignored.");
        return;
    }
    s_futureSynchronizer->waitForFinished();
}

void QThreadFunctionBase::storeFuture(const QFuture<void> &future)
{
    Q_D(QThreadFunctionBase);
    d->m_future = future;
    if (d->isUsingSynchronizer())
        s_futureSynchronizer->addFuture(future);
}

/*!
    \typedef QThreadFunctionTask
    \relates QCustomTask

    Type alias for the QCustomTask<QThreadFunction<ResultType>>,
    to be used inside recipes.
*/

QT_END_NAMESPACE
