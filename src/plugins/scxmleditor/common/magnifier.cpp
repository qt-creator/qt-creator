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

#include "magnifier.h"
#include "graphicsscene.h"
#include "graphicsview.h"

#include <QMouseEvent>

using namespace ScxmlEditor::Common;

Magnifier::Magnifier(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
    setMouseTracking(true);
    m_ui.m_graphicsView->setEnabled(false);
}

void Magnifier::setTopLeft(const QPoint &topLeft)
{
    m_topLeft = topLeft;
}

void Magnifier::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    qreal radius = width() / 2.0;
    m_gradientBrush.setCenter(radius, radius);
    m_gradientBrush.setFocalPoint(radius, radius);
    m_gradientBrush.setRadius(radius);
    m_gradientBrush.setColorAt(1.0, QColor(255, 255, 255, 0));
    m_gradientBrush.setColorAt(0.0, QColor(0, 0, 0, 255));

    int cap = radius * 0.1;
    m_ui.m_graphicsView->setMask(QRegion(rect().adjusted(cap, cap, -cap, -cap), QRegion::Ellipse));
}

void Magnifier::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    grabMouse();
}

void Magnifier::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);
    releaseMouse();
}

void Magnifier::mousePressEvent(QMouseEvent *e)
{
    QWidget::mousePressEvent(e);
    if (m_mainView)
        m_mainView->magnifierClicked(m_ui.m_graphicsView->transform().m11(),
            m_ui.m_graphicsView->mapToScene(e->pos() - m_topLeft + rect().center()));
}

void Magnifier::mouseMoveEvent(QMouseEvent *e)
{
    QWidget::mouseMoveEvent(e);
    move(pos() + (e->pos() - rect().center()));
}

void Magnifier::wheelEvent(QWheelEvent *e)
{
    QWidget::wheelEvent(e);

    if (e->delta() > 0)
        m_ui.m_graphicsView->scale(1.1, 1.1);
    else
        m_ui.m_graphicsView->scale(1.0 / 1.1, 1.0 / 1.1);

    if (m_mainView)
        m_ui.m_graphicsView->centerOn(m_mainView->mapToScene(pos() - m_topLeft + rect().center()));
}

void Magnifier::moveEvent(QMoveEvent *e)
{
    QWidget::moveEvent(e);

    if (m_mainView)
        m_ui.m_graphicsView->centerOn(m_mainView->mapToScene(e->pos() - m_topLeft + rect().center()));
}

void Magnifier::setCurrentView(GraphicsView *view)
{
    m_mainView = view;
}

void Magnifier::setCurrentScene(ScxmlEditor::PluginInterface::GraphicsScene *scene)
{
    m_ui.m_graphicsView->setScene(scene);
}

void Magnifier::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter p(this);

    p.setPen(Qt::NoPen);
    p.setBrush(m_gradientBrush);
    p.drawRect(rect());
}
