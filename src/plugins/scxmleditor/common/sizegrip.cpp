// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    m_startPoint = e->globalPosition().toPoint();
    m_startRect = parentWidget()->rect();
    m_mouseDown = true;
    checkCursor(e->pos());
}

void SizeGrip::mouseMoveEvent(QMouseEvent *e)
{
    if (m_mouseDown) {
        QPoint p = e->globalPosition().toPoint() - m_startPoint;
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
