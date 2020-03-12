/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "overlaywidget.h"

#include "qtcassert.h"

#include <QEvent>
#include <QPainter>

Utils::OverlayWidget::OverlayWidget(QWidget *parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    if (parent)
        attachToWidget(parent);
}

void Utils::OverlayWidget::setPaintFunction(const Utils::OverlayWidget::PaintFunction &paint)
{
    m_paint = paint;
}

bool Utils::OverlayWidget::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent() && ev->type() == QEvent::Resize)
        resizeToParent();
    return QWidget::eventFilter(obj, ev);
}

void Utils::OverlayWidget::paintEvent(QPaintEvent *ev)
{
    if (m_paint) {
        QPainter p(this);
        m_paint(this, p, ev);
    }
}

void Utils::OverlayWidget::attachToWidget(QWidget *parent)
{
    if (parentWidget())
        parentWidget()->removeEventFilter(this);
    setParent(parent);
    if (parent) {
        parent->installEventFilter(this);
        resizeToParent();
        raise();
    }
}

void Utils::OverlayWidget::resizeToParent()
{
    QTC_ASSERT(parentWidget(), return );
    setGeometry(QRect(QPoint(0, 0), parentWidget()->size()));
}
