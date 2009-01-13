/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "progressmanager_p.h"
#include "progressview.h"
#include "coreimpl.h"
#include "baseview.h"

#include "coreconstants.h"
#include "uniqueidmanager.h"

#include <utils/qtcassert.h>

using namespace Core;
using namespace Core::Internal;

ProgressManagerPrivate::ProgressManagerPrivate(QObject *parent)
  : ProgressManagerInterface(parent)
{
    m_progressView = new ProgressView;
    ICore *core = CoreImpl::instance();
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
