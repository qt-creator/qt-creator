/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "progressmanager_p.h"
#include "progressview.h"
#include "baseview.h"
#include "coreconstants.h"
#include "icore.h"
#include "uniqueidmanager.h"

#include <utils/qtcassert.h>

using namespace Core;
using namespace Core::Internal;

ProgressManagerPrivate::ProgressManagerPrivate(QObject *parent)
  : ProgressManager(parent)
{
    m_progressView = new ProgressView;
    ICore *core = ICore::instance();
    connect(core, SIGNAL(coreAboutToClose()), this, SLOT(cancelAllRunningTasks()));
}

ProgressManagerPrivate::~ProgressManagerPrivate()
{
}

void ProgressManagerPrivate::init()
{
}

void ProgressManagerPrivate::cancelTasks(const QString &type)
{
    QMap<QFutureWatcher<void> *, QString>::iterator task = m_runningTasks.begin();
    while (task != m_runningTasks.end()) {
        if (task.value() != type) {
            ++task;
            continue;
        }
        disconnect(task.key(), SIGNAL(finished()), this, SLOT(taskFinished()));
        task.key()->cancel();
        delete task.key();
        task = m_runningTasks.erase(task);
    }
}

void ProgressManagerPrivate::cancelAllRunningTasks()
{
    QMap<QFutureWatcher<void> *, QString>::const_iterator task = m_runningTasks.constBegin();
    while (task != m_runningTasks.constEnd()) {
        disconnect(task.key(), SIGNAL(finished()), this, SLOT(taskFinished()));
        task.key()->cancel();
        delete task.key();
        ++task;
    }
    m_runningTasks.clear();
}

FutureProgress *ProgressManagerPrivate::addTask(const QFuture<void> &future, const QString &title, const QString &type, PersistentType persistency)
{
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>();
    m_runningTasks.insert(watcher, type);
    connect(watcher, SIGNAL(finished()), this, SLOT(taskFinished()));
    watcher->setFuture(future);
    return m_progressView->addTask(future, title, type, persistency);
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
    m_runningTasks.remove(task);
    delete task;
}
