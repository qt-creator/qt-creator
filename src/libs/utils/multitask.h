/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MULTITASK_H
#define MULTITASK_H

#include "utils_global.h"
#include "runextensions.h"

#include <QObject>
#include <QList>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QThreadPool>

#include <QDebug>

QT_BEGIN_NAMESPACE

namespace QtConcurrent {

class QTCREATOR_UTILS_EXPORT MultiTaskBase : public QObject, public QRunnable
{
    Q_OBJECT
protected slots:
    virtual void cancelSelf() = 0;
    virtual void setFinished() = 0;
    virtual void setProgressRange(int min, int max) = 0;
    virtual void setProgressValue(int value) = 0;
    virtual void setProgressText(QString value) = 0;
};

template <typename Class, typename R>
class MultiTask : public MultiTaskBase
{
public:
    MultiTask(void (Class::*fn)(QFutureInterface<R> &), const QList<Class *> &objects)
        : fn(fn),
        objects(objects)
    {
        maxProgress = 100*objects.size();
    }

    QFuture<R> future()
    {
        futureInterface.reportStarted();
        return futureInterface.future();
    }

    void run()
    {
        QThreadPool::globalInstance()->releaseThread();
        futureInterface.setProgressRange(0, maxProgress);
        foreach (Class *object, objects) {
            QFutureWatcher<R> *watcher = new QFutureWatcher<R>();
            watchers.insert(object, watcher);
            finished.insert(watcher, false);
            connect(watcher, &QFutureWatcherBase::finished,
                    this, &MultiTask::setFinished);
            connect(watcher, &QFutureWatcherBase::progressRangeChanged,
                    this, &MultiTask::setProgressRange);
            connect(watcher, &QFutureWatcherBase::progressValueChanged,
                    this, &MultiTask::setProgressValue);
            connect(watcher, &QFutureWatcherBase::progressTextChanged,
                    this, &MultiTask::setProgressText);
            watcher->setFuture(QtConcurrent::run(fn, object));
        }
        selfWatcher = new QFutureWatcher<R>();
        connect(selfWatcher, &QFutureWatcherBase::canceled, this, &MultiTask::cancelSelf);
        selfWatcher->setFuture(futureInterface.future());
        loop = new QEventLoop;
        loop->exec();
        futureInterface.reportFinished();
        QThreadPool::globalInstance()->reserveThread();
        qDeleteAll(watchers);
        delete selfWatcher;
        delete loop;
    }
protected:
    void cancelSelf()
    {
        foreach (QFutureWatcher<R> *watcher, watchers)
            watcher->future().cancel();
    }

    void setFinished()
    {
        updateProgress();
        QFutureWatcher<R> *watcher = static_cast<QFutureWatcher<R> *>(sender());
        if (finished.contains(watcher))
            finished[watcher] = true;
        bool allFinished = true;
        foreach (bool isFinished, finished) {
            if (!isFinished) {
                allFinished = false;
                break;
            }
        }
        if (allFinished)
            loop->quit();
    }

    void setProgressRange(int min, int max)
    {
        Q_UNUSED(min)
        Q_UNUSED(max)
        updateProgress();
    }

    void setProgressValue(int value)
    {
        Q_UNUSED(value)
        updateProgress();
    }

    void setProgressText(QString value)
    {
        Q_UNUSED(value)
        updateProgressText();
    }
private:
    void updateProgress()
    {
        int progressSum = 0;
        foreach (QFutureWatcher<R> *watcher, watchers) {
            if (watcher->progressMinimum() == watcher->progressMaximum()) {
                if (watcher->future().isFinished() && !watcher->future().isCanceled())
                    progressSum += 100;
            } else {
                progressSum += 100*(watcher->progressValue()-watcher->progressMinimum())/(watcher->progressMaximum()-watcher->progressMinimum());
            }
        }
        futureInterface.setProgressValue(progressSum);
    }

    void updateProgressText()
    {
        QString text;
        foreach (QFutureWatcher<R> *watcher, watchers) {
            if (!watcher->progressText().isEmpty()) {
                text += watcher->progressText();
                text += QLatin1Char('\n');
            }
        }
        text = text.trimmed();
        futureInterface.setProgressValueAndText(futureInterface.progressValue(), text);
    }

    QFutureInterface<R> futureInterface;
    void (Class::*fn)(QFutureInterface<R> &);
    QList<Class *> objects;

    QFutureWatcher<R> *selfWatcher;
    QMap<Class *, QFutureWatcher<R> *> watchers;
    QMap<QFutureWatcher<R> *, bool> finished;
    QEventLoop *loop;
    int maxProgress;
};

template <typename Class, typename T>
QFuture<T> run(void (Class::*fn)(QFutureInterface<T> &), const QList<Class *> &objects, int priority = 0)
{
    MultiTask<Class, T> *task = new MultiTask<Class, T>(fn, objects);
    QFuture<T> future = task->future();
    QThreadPool::globalInstance()->start(task, priority);
    return future;
}

} // namespace QtConcurrent

QT_END_NAMESPACE

#endif // MULTITASK_H
