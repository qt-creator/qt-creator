// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTaskTree/qprocesstask.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QWaitCondition>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(process)

static bool isMainThread()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    return QThread::isMainThread();
#else
    return QThread::currentThread() == qApp->thread();
#endif
}

using namespace QtTaskTree;

namespace QtTaskTree {

class ProcessReaperPrivate;

class ProcessReaper final
{
public:
    static void reap(QProcess *process, int timeoutMs = 500);
    ProcessReaper();
    ~ProcessReaper();

private:
    static ProcessReaper *instance();

    QThread m_thread;
    ProcessReaperPrivate *m_private;
};

static const int s_timeoutThreshold = 10000; // 10 seconds

static QString execWithArguments(QProcess *process)
{
    QStringList commandLine;
    commandLine.append(process->program());
    commandLine.append(process->arguments());
    return commandLine.join(QChar::Space);
}

struct ReaperSetup
{
    QProcess *m_process = nullptr;
    int m_timeoutMs;
};

class Reaper : public QObject
{
    Q_OBJECT

public:
    Reaper(const ReaperSetup &reaperSetup) : m_reaperSetup(reaperSetup) {}

    void reap()
    {
        m_timer.start();
        connect(m_reaperSetup.m_process, &QProcess::finished, this, &Reaper::handleFinished);
        if (emitFinished())
            return;
        terminate();
    }

Q_SIGNALS:
    void finished();

private:
    void terminate()
    {
        m_reaperSetup.m_process->terminate();
        QTimer::singleShot(m_reaperSetup.m_timeoutMs, this, &Reaper::handleTerminateTimeout);
    }

    void kill() { m_reaperSetup.m_process->kill(); }

    bool emitFinished()
    {
        if (m_reaperSetup.m_process->state() != QProcess::NotRunning)
            return false;

        if (!m_finished) {
            const int timeout = m_timer.elapsed();
            if (timeout > s_timeoutThreshold) {
                qWarning() << "Finished parallel reaping of" << execWithArguments(m_reaperSetup.m_process)
                           << "in" << (timeout / 1000.0) << "seconds.";
            }

            m_finished = true;
            Q_EMIT finished();
        }
        return true;
    }

    void handleFinished()
    {
        if (emitFinished())
            return;
        qWarning() << "Finished process still running...";
        // In case the process is still running - wait until it has finished
        QTimer::singleShot(m_reaperSetup.m_timeoutMs, this, &Reaper::handleFinished);
    }

    void handleTerminateTimeout()
    {
        if (emitFinished())
            return;
        kill();
    }

    bool m_finished = false;
    QElapsedTimer m_timer;
    const ReaperSetup m_reaperSetup;
};

class ProcessReaperPrivate : public QObject
{
    Q_OBJECT

public:
    // Called from non-reaper's thread
    void scheduleReap(const ReaperSetup &reaperSetup)
    {
        Q_ASSERT_X(QThread::currentThread() != thread(), "ProcessReaperPrivate",
                   "Can't schedule reap from the reaper internal thread.");

        QMutexLocker locker(&m_mutex);
        m_reaperSetupList.append(reaperSetup);
        QMetaObject::invokeMethod(this, &ProcessReaperPrivate::flush);
    }

    // Called from non-reaper's thread
    void waitForFinished()
    {
        Q_ASSERT_X(QThread::currentThread() != thread(), "ProcessReaperPrivate",
                   "Can't wait for finished from the reaper internal thread.");

        QMetaObject::invokeMethod(this, &ProcessReaperPrivate::flush,
                                  Qt::BlockingQueuedConnection);
        QMutexLocker locker(&m_mutex);
        if (m_reaperList.isEmpty())
            return;

        m_waitCondition.wait(&m_mutex);
    }

private:
    // All the private methods are called from the reaper's thread
    QList<ReaperSetup> takeReaperSetupList()
    {
        QMutexLocker locker(&m_mutex);
        return std::exchange(m_reaperSetupList, {});
    }

    void flush()
    {
        while (true) {
            const QList<ReaperSetup> reaperSetupList = takeReaperSetupList();
            if (reaperSetupList.isEmpty())
                return;
            for (const ReaperSetup &reaperSetup : reaperSetupList)
                reap(reaperSetup);
        }
    }

    void reap(const ReaperSetup &reaperSetup)
    {
        Reaper *reaper = new Reaper(reaperSetup);
        connect(reaper, &Reaper::finished, this, [this, reaper, process = reaperSetup.m_process] {
            QMutexLocker locker(&m_mutex);
            const bool isRemoved = m_reaperList.removeOne(reaper);
            if (!isRemoved)
                qWarning() << "Reaper list doesn't contain the finished process.";

            delete reaper;
            delete process;
            if (m_reaperList.isEmpty())
                m_waitCondition.wakeOne();
        }, Qt::QueuedConnection);

        {
            QMutexLocker locker(&m_mutex);
            m_reaperList.append(reaper);
        }

        reaper->reap();
    }

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    QList<ReaperSetup> m_reaperSetupList;
    QList<Reaper *> m_reaperList;
};

static ProcessReaper *s_instance = nullptr;
static QMutex s_instanceMutex;

// Call me with s_instanceMutex locked.
ProcessReaper *ProcessReaper::instance()
{
    if (!s_instance)
        s_instance = new ProcessReaper;
    return s_instance;
}

ProcessReaper::ProcessReaper()
    : m_private(new ProcessReaperPrivate)
{
    m_private->moveToThread(&m_thread);
    QObject::connect(&m_thread, &QThread::finished, m_private, &QObject::deleteLater);
    m_thread.start();
    m_thread.moveToThread(qApp->thread());
}

ProcessReaper::~ProcessReaper()
{
    Q_ASSERT_X(isMainThread(), "ProcessReaper",
               "Destructing process reaper from non-main thread.");

    instance()->m_private->waitForFinished();
    m_thread.quit();
    m_thread.wait();
}

void ProcessReaper::reap(QProcess *process, int timeoutMs)
{
    if (!process)
        return;

    if (QThread::currentThread() != process->thread()) {
        qWarning() << "Process can only be reaped from its own thread.";
        return;
    }

    process->disconnect();
    if (process->state() == QProcess::NotRunning) {
        delete process;
        return;
    }

    // Neither can move object with a parent into a different thread
    // nor reaping the process with a parent makes any sense.
    process->setParent(nullptr);

    QMutexLocker locker(&s_instanceMutex);
    ProcessReaperPrivate *priv = instance()->m_private;

    process->moveToThread(priv->thread());
    const ReaperSetup reaperSetup {process, timeoutMs};
    priv->scheduleReap(reaperSetup);
}

} // namespace QtTaskTree

/*!
    \class QProcessDeleter
    \inheaderfile qprocesstask.h
    \inmodule TaskTree
    \brief A custom deleter for QProcess, used by QProcessTask.
    \reentrant

    QProcessDeleter is used to enable fast destruction of running
    QProcess instances via QProcessTask in QTaskTree. Instead of deleting
    the running QProcess, which may block the caller's thread execution
    for a long time, the QProcessDeleter moves the running QProcess into
    a separate thread and tries to finish it in a most gentle way.
    This consist of calling QProcess::terminate() with \c {500 ms} timeout,
    and if the process is still running after this timeout passed,
    the additional call to QProcess::kill() is performed.

    Finally, on application quit, QProcessDeleter::syncAll() should be called
    in order to synchronize all processes being still potentially finalized
    in separate thread. The call to QProcessDeleter::syncAll() is blocking
    in case some processes are still being finalized.

    This strategy seems sensible, since when deleting the running QProcess via
    QProcessDeleter we don't block immediately, but postpone the possible
    (but not certain) block until the end of an application.

    It is used by QProcessTask.
*/

/*!
    This method should be called on application quit to synchronize the
    finalization of all still possibly running QProcess instances that were
    deleted before using QProcessDeleter. The call should be executed from
    the main thread.
*/
void QProcessDeleter::syncAll()
{
    if (!isMainThread()) {
        qWarning("The QProcessDeleter::syncAll() should be called from the main thread. "
                 "The current call is ignored.");
        return;
    }
    QMutexLocker locker(&s_instanceMutex);
    delete s_instance;
    s_instance = nullptr;
}

/*!
    Deletes the passed \a process in a most safe way.
    If the passed \a process is not running, it is deleted directly.
    Otherwise, it moves the \a process into a separate thread and
    tries to finish it in a most safe way by calling QProcess::terminate()
    with \c {500 ms} timeout, and if the process is still running after
    this timeout passed, the additional call to QProcess::kill() is performed.
*/
void QProcessDeleter::operator()(QProcess *process)
{
    ProcessReaper::reap(process);
}

void QProcessTaskAdapter::operator()(QProcess *task, QTaskInterface *iface)
{
    QObject::connect(task, &QProcess::finished, iface, [iface, task] {
        const bool success = task->exitStatus() == QProcess::NormalExit
                             && task->error() == QProcess::UnknownError
                             && task->exitCode() == 0;
        iface->reportDone(QtTaskTree::toDoneResult(success));
    });
    QObject::connect(task, &QProcess::errorOccurred, iface, [iface](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        iface->reportDone(QtTaskTree::DoneResult::Error);
    });
    task->start();
}

/*!
    \typedef QProcessTask
    \relates QCustomTask

    Type alias for the QCustomTask<QProcess>, using QProcessDeleter,
    to be used inside recipes.
*/

#endif // QT_CONFIG(process)

QT_END_NAMESPACE

#if QT_CONFIG(process)

#include "qprocesstask.moc"

#endif // QT_CONFIG(process)

