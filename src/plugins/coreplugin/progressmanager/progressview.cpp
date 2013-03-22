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

#include "progressview.h"
#include "futureprogress.h"

#include <utils/qtcassert.h>

#include <QEvent>
#include <QVBoxLayout>

using namespace Core;
using namespace Core::Internal;

static const int PROGRESS_WIDTH = 100;

ProgressView::ProgressView(QWidget *parent)
    : QWidget(parent), m_referenceWidget(0), m_hovered(false)
{
    m_layout = new QVBoxLayout;
    setLayout(m_layout);
    m_layout->setContentsMargins(0, 0, 0, 1);
    m_layout->setSpacing(0);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
    setWindowTitle(tr("Processes"));
}

ProgressView::~ProgressView()
{
    qDeleteAll(m_taskList);
    m_taskList.clear();
}

FutureProgress *ProgressView::addTask(const QFuture<void> &future,
                                      const QString &title,
                                      const QString &type,
                                      ProgressManager::ProgressFlags flags)
{
    removeOldTasks(type);
    if (m_taskList.size() == 10)
        removeOneOldTask();
    FutureProgress *progress = new FutureProgress(this);
    progress->setTitle(title);
    progress->setFuture(future);

    m_layout->insertWidget(0, progress);
    m_taskList.append(progress);
    progress->setType(type);
    if (flags.testFlag(ProgressManager::KeepOnFinish))
        progress->setKeepOnFinish(FutureProgress::KeepOnFinishTillUserInteraction);
    else
        progress->setKeepOnFinish(FutureProgress::HideOnFinish);
    connect(progress, SIGNAL(hasErrorChanged()), this, SIGNAL(hasErrorChanged()));
    connect(progress, SIGNAL(removeMe()), this, SLOT(slotRemoveTask()));
    connect(progress, SIGNAL(fadeStarted()), this, SLOT(checkForLastProgressFading()));
    return progress;
}

bool ProgressView::hasError() const
{
    foreach (FutureProgress *progress, m_taskList)
        if (progress->hasError())
            return true;
    return false;
}

bool ProgressView::isFading() const
{
    if (m_taskList.isEmpty())
        return false;
    foreach (FutureProgress *progress, m_taskList) {
        if (!progress->isFading()) // we still have progress bars that are not fading, do nothing
            return false;
    }
    return true;
}

bool ProgressView::isEmpty() const
{
    return m_taskList.isEmpty();
}

bool ProgressView::isHovered() const
{
    return m_hovered;
}

void ProgressView::setReferenceWidget(QWidget *widget)
{
    if (m_referenceWidget)
        removeEventFilter(this);
    m_referenceWidget = widget;
    if (m_referenceWidget)
        installEventFilter(this);
    reposition();
}

bool ProgressView::event(QEvent *event)
{
    if (event->type() == QEvent::ParentAboutToChange && parentWidget()) {
        parentWidget()->removeEventFilter(this);
    } else if (event->type() == QEvent::ParentChange && parentWidget()) {
        parentWidget()->installEventFilter(this);
    } else if (event->type() == QEvent::Resize) {
        reposition();
    } else if (event->type() == QEvent::Enter) {
        m_hovered = true;
        emit hoveredChanged(m_hovered);
    } else if (event->type() == QEvent::Leave) {
        m_hovered = false;
        emit hoveredChanged(m_hovered);
    }
    return QWidget::event(event);
}

bool ProgressView::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == parentWidget() || obj == m_referenceWidget) && event->type() == QEvent::Resize)
        reposition();
    return false;
}

void ProgressView::removeOldTasks(const QString &type, bool keepOne)
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
}

void ProgressView::deleteTask(FutureProgress *progress)
{
    layout()->removeWidget(progress);
    progress->hide();
    progress->deleteLater();
}

void ProgressView::reposition()
{
    if (!parentWidget() || !m_referenceWidget)
        return;
    QPoint topRightReferenceInParent =
            m_referenceWidget->mapTo(parentWidget(), m_referenceWidget->rect().topRight());
    move(topRightReferenceInParent - rect().bottomRight());
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
}

void ProgressView::removeTask(FutureProgress *task)
{
    m_taskList.removeAll(task);
    deleteTask(task);
}

void ProgressView::slotRemoveTask()
{
    FutureProgress *progress = qobject_cast<FutureProgress *>(sender());
    QTC_ASSERT(progress, return);
    QString type = progress->type();
    removeTask(progress);
    removeOldTasks(type, true);
}

void ProgressView::checkForLastProgressFading()
{
    if (isEmpty() || isFading())
        emit fadeOfLastProgressStarted();
}
