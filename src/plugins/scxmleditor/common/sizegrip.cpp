/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "sizegrip.h"
#include <QMouseEvent>
#include <QPainter>

using namespace ScxmlEditor::Common;

SizeGrip::SizeGrip(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

void SizeGrip::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    QRect r = rect().adjusted(2, 2, -2, -2);
    m_pol = QPolygon() << r.topRight() << r.bottomRight() << r.bottomLeft();
}

void SizeGrip::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
    m_startPoint = e->globalPos();
    m_startRect = parentWidget()->rect();
    m_mouseDown = true;
    checkCursor(e->pos());
}

void SizeGrip::mouseMoveEvent(QMouseEvent *e)
{
    if (m_mouseDown) {
        QPoint p = e->globalPos() - m_startPoint;
        parentWidget()->resize(m_startRect.width() + p.x(), m_startRect.height() + p.y());
    } else {
        checkCursor(e->pos());
    }
}

void SizeGrip::mouseReleaseEvent(QMouseEvent *e)
{
    QWidget::mouseReleaseEvent(e);
    m_mouseDown = false;
}

void SizeGrip::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QRect r = rect();
    int c = r.width() * 0.33;

    QPainter p(this);
    p.setPen(Qt::gray);
    p.drawLine(r.bottomLeft() + QPoint(0, -2), r.topRight() + QPoint(-2, 0));
    p.drawLine(r.bottomLeft() + QPoint(c, -2), r.topRight() + QPoint(-2, c));
    p.drawLine(r.bottomLeft() + QPoint(2 * c, -2), r.topRight() + QPoint(-2, 2 * c));
}

void SizeGrip::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);
    unsetCursor();
}

void SizeGrip::checkCursor(const QPoint &p)
{
    if (m_pol.containsPoint(p, Qt::OddEvenFill))
        setCursor(Qt::SizeFDiagCursor);
    else
        unsetCursor();
}
