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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "progressview.h"
#include "futureprogress.h"

#include <QtGui/QHBoxLayout>

using namespace Core;
using namespace Core::Internal;

ProgressView::ProgressView(QWidget *parent)
: QWidget(parent)
{
    m_layout = new QVBoxLayout;
    setLayout(m_layout);
    m_layout->setMargin(0);
    m_layout->setSpacing(0);
    setWindowTitle(tr("Processes"));
}

ProgressView::~ProgressView()
{
    qDeleteAll(m_taskList);
    m_taskList.clear();
    m_type.clear();
    m_keep.clear();
}

FutureProgress *ProgressView::addTask(const QFuture<void> &future,
                                      const QString &title,
                                      const QString &type,
                                      ProgressManagerInterface::PersistentType persistency)
{
    removeOldTasks(type);
    if (m_taskList.size() == 3)
        removeOneOldTask();
    FutureProgress *progress = new FutureProgress(this);
    progress->setTitle(title);
    progress->setFuture(future);
    m_layout->insertWidget(0, progress);
    m_taskList.append(progress);
    m_type.insert(progress, type);
    m_keep.insert(progress, (persistency == ProgressManagerInterface::KeepOnFinish));
    connect(progress, SIGNAL(finished()), this, SLOT(slotFinished()));
    return progress;
}

void ProgressView::removeOldTasks(const QString &type, bool keepOne)
{
    bool firstFound = !keepOne; // start with false if we want to keep one
    QList<FutureProgress *>::iterator i = m_taskList.end();
    while (i != m_taskList.begin()) {
        --i;
        if (m_type.value(*i) == type) {
            if (firstFound && (*i)->future().isFinished()) {
                deleteTask(*i);
                i = m_taskList.erase(i);
            }
            firstFound = true;
        }
    }
}

void ProgressView::deleteTask(FutureProgress *progress)
{
    m_type.remove(progress);
    m_keep.remove(progress);
    layout()->removeWidget(progress);
    progress->deleteLater();
}

void ProgressView::removeOneOldTask()
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
        QString type = m_type.value(*i);
        if (m_type.keys(type).size() > 1) { // don't care for optimizations it's only a handful of entries
            deleteTask(*i);
            i = m_taskList.erase(i);
            return;
        }
    }

    // no ended process, no type with multiple processes, just remove the oldest task
    FutureProgress *task = m_taskList.takeFirst();
    deleteTask(task);
}

void ProgressView::removeTask(FutureProgress *task)
{
    m_taskList.removeAll(task);
    deleteTask(task);
}

void ProgressView::slotFinished()
{
    FutureProgress *progress = qobject_cast<FutureProgress *>(sender());
    Q_ASSERT(progress);
    if (m_keep.contains(progress) && !m_keep.value(progress) && !progress->hasError())
        removeTask(progress);
    removeOldTasks(m_type.value(progress), true);
}
