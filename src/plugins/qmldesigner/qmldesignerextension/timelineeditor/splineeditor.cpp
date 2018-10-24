/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "splineeditor.h"

#include <theme.h>

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QResizeEvent>

namespace QmlDesigner {

SplineEditor::SplineEditor(QWidget *parent)
    : QWidget(parent)
    , m_canvas(0, 0, 25, 25, 9, 6, 0, 1)
    , m_animation(new QPropertyAnimation(this, "progress"))
{
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setLoopCount(1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

double SplineEditor::progress() const
{
    return m_progress;
}

EasingCurve SplineEditor::easingCurve() const
{
    return m_curve;
}

void SplineEditor::animate() const
{
    m_animation->start();
}

void SplineEditor::setDuration(int duration)
{
    m_animation->setDuration(duration);
}

void SplineEditor::setProgress(double progress)
{
    m_progress = progress;
    update();
}

void SplineEditor::setEasingCurve(const EasingCurve &curve)
{
    m_curve = curve;
    update();
}

void SplineEditor::resizeEvent(QResizeEvent *event)
{
    m_canvas.resize(event->size());
    QWidget::resizeEvent(event);
}

void SplineEditor::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPen pen(Qt::black);
    pen.setWidth(1);
    painter.drawRect(0, 0, width() - 1, height() - 1);

    painter.setRenderHint(QPainter::Antialiasing);

    pen = QPen(Qt::darkGray);
    pen.setWidth(1);
    painter.setPen(pen);

    QColor curveColor = Qt::white;
    if (!m_curve.isLegal())
        curveColor = Qt::red;

    QBrush background(Theme::instance()->qmlDesignerBackgroundColorDarker());
    m_canvas.paintGrid(&painter, background);
    m_canvas.paintCurve(&painter, m_curve, curveColor);
    m_canvas.paintControlPoints(&painter, m_curve);

    if (m_animation->state() == QAbstractAnimation::Running)
        m_canvas.paintProgress(&painter, m_curve, m_progress);
}

void SplineEditor::mousePressEvent(QMouseEvent *e)
{
    bool clearActive = true;
    if (e->button() == Qt::LeftButton) {
        EasingCurve mappedCurve = m_canvas.mapTo(m_curve);
        int active = mappedCurve.hit(e->pos(), 10);

        if (EasingCurve::IsValidIndex(active)) {
            clearActive = false;
            m_curve.setActive(active);
            mouseMoveEvent(e);
        }

        m_mousePress = e->pos();
        e->accept();
    }

    if (clearActive) {
        m_curve.clearActive();
        update();
    }
}

void SplineEditor::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_mouseDrag = false;
        e->accept();
    }
}

void dragHandle(EasingCurve &curve, int id, const QPointF &pos)
{
    QPointF distance = pos - curve.point(id);

    curve.setPoint(id, pos);

    if (curve.isLeftHandle(id))
        curve.movePoint(id + 2, -distance);
    else
        curve.movePoint(id - 2, -distance);
}

void SplineEditor::mouseMoveEvent(QMouseEvent *e)
{
    // If we've moved more then 25 pixels, assume user is dragging
    if (!m_mouseDrag
        && QPoint(m_mousePress - e->pos()).manhattanLength() > qApp->startDragDistance())
        m_mouseDrag = true;

    if (m_mouseDrag && m_curve.hasActive()) {
        QPointF p = m_canvas.mapFrom(e->pos());
        int active = m_curve.active();

        if ((active == 0 || active == m_curve.count() - 2)
            && e->modifiers().testFlag(Qt::ShiftModifier)) {
            if (active == 0) {
                QPointF opposite = QPointF(1.0, 1.0) - p;
                dragHandle(m_curve, active, p);
                dragHandle(m_curve, m_curve.count() - 2, opposite);

            } else {
                QPointF opposite = QPointF(1.0, 1.0) - p;
                dragHandle(m_curve, active, p);
                dragHandle(m_curve, 0, opposite);
            }

        } else if (m_curve.isHandle(active)) {
            int poc = m_curve.curvePoint(active);

            if (!m_curve.isSmooth(poc))
                m_curve.setPoint(active, p);
            else
                dragHandle(m_curve, active, p);

        } else {
            QPointF targetPoint = p;
            QPointF distance = targetPoint - m_curve.point(m_curve.active());

            m_curve.setPoint(active, targetPoint);
            m_curve.movePoint(active + 1, distance);
            m_curve.movePoint(active - 1, distance);
        }

        update();
        emit easingCurveChanged(m_curve);
    }
}

void SplineEditor::contextMenuEvent(QContextMenuEvent *e)
{
    m_curve.clearActive();

    QMenu menu;

    EasingCurve mappedCurve = m_canvas.mapTo(m_curve);
    int index = mappedCurve.hit(e->pos(), 10);

    if (index > 0 && !m_curve.isHandle(index)) {
        QAction *deleteAction = menu.addAction(tr("Delete Point"));
        connect(deleteAction, &QAction::triggered, [this, index]() {
            m_curve.deletePoint(index);
            update();
            emit easingCurveChanged(m_curve);
        });

        QAction *smoothAction = menu.addAction(tr("Smooth Point"));
        smoothAction->setCheckable(true);
        smoothAction->setChecked(m_curve.isSmooth(index));
        connect(smoothAction, &QAction::triggered, [this, index]() {
            m_curve.makeSmooth(index);
            update();
            emit easingCurveChanged(m_curve);
        });

        QAction *cornerAction = menu.addAction(tr("Corner Point"));
        connect(cornerAction, &QAction::triggered, [this, index]() {
            m_curve.breakTangent(index);
            update();
            emit easingCurveChanged(m_curve);
        });

    } else {
        QAction *addAction = menu.addAction(tr("Add Point"));
        connect(addAction, &QAction::triggered, [&]() {
            m_curve.addPoint(m_canvas.mapFrom(e->pos()));
            m_curve.makeSmooth(m_curve.active());
            update();
            emit easingCurveChanged(m_curve);
        });
    }

    QAction *zoomAction = menu.addAction(tr("Reset Zoom"));
    connect(zoomAction, &QAction::triggered, [&]() {
        m_canvas.setScale(1.0);
        update();
    });

    menu.exec(e->globalPos());
    e->accept();
}

void SplineEditor::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_animation->start();
    QWidget::mouseDoubleClickEvent(event);
}

void SplineEditor::wheelEvent(QWheelEvent *event)
{
    double tmp = event->angleDelta().y() > 0 ? 0.05 : -0.05;

    m_canvas.setScale(m_canvas.scale() + tmp);
    event->accept();
    update();
}

} // End namespace QmlDesigner.
