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
}

void ProgressView::addProgressWidget(QWidget *widget)
{
    m_layout->insertWidget(0, widget);
}

void ProgressView::removeProgressWidget(QWidget *widget)
{
    m_layout->removeWidget(widget);
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

void ProgressView::reposition()
{
    if (!parentWidget() || !m_referenceWidget)
        return;
    QPoint topRightReferenceInParent =
            m_referenceWidget->mapTo(parentWidget(), m_referenceWidget->rect().topRight());
    move(topRightReferenceInParent - rect().bottomRight());
}
