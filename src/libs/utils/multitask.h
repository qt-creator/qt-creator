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
            connect(watcher, SIGNAL(finished()), this, SLOT(setFinished()));
            connect(watcher, SIGNAL(progressRangeChanged(int,int)), this, SLOT(setProgressRange(int,int)));
            connect(watcher, SIGNAL(progressValueChanged(int)), this, SLOT(setProgressValue(int)));
            connect(watcher, SIGNAL(progressTextChanged(QString)), this, SLOT(setProgressText(QString)));
            watcher->setFuture(QtConcurrent::run(fn, object));
        }
        selfWatcher = new QFutureWatcher<R>();
        connect(selfWatcher, SIGNAL(canceled()), this, SLOT(cancelSelf()));
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
