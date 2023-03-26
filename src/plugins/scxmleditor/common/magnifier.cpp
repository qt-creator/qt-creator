// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "magnifier.h"
#include "graphicsscene.h"
#include "graphicsview.h"

#include <QMouseEvent>
#include <QVBoxLayout>

using namespace ScxmlEditor::Common;

Magnifier::Magnifier(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    m_graphicsView = new QGraphicsView(this);
    m_graphicsView->setInteractive(false);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setEnabled(false);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_graphicsView);
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
    m_graphicsView->setMask(QRegion(rect().adjusted(cap, cap, -cap, -cap), QRegion::Ellipse));
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
        m_mainView->magnifierClicked(m_graphicsView->transform().m11(),
            m_graphicsView->mapToScene(e->pos() - m_topLeft + rect().center()));
}

void Magnifier::mouseMoveEvent(QMouseEvent *e)
{
    QWidget::mouseMoveEvent(e);
    move(pos() + (e->pos() - rect().center()));
}

void Magnifier::wheelEvent(QWheelEvent *e)
{
    QWidget::wheelEvent(e);

    if (e->angleDelta().y() > 0)
        m_graphicsView->scale(1.1, 1.1);
    else
        m_graphicsView->scale(1.0 / 1.1, 1.0 / 1.1);

    if (m_mainView)
        m_graphicsView->centerOn(m_mainView->mapToScene(pos() - m_topLeft + rect().center()));
}

void Magnifier::moveEvent(QMoveEvent *e)
{
    QWidget::moveEvent(e);

    if (m_mainView)
        m_graphicsView->centerOn(m_mainView->mapToScene(e->pos() - m_topLeft + rect().center()));
}

void Magnifier::setCurrentView(GraphicsView *view)
{
    m_mainView = view;
}

void Magnifier::setCurrentScene(ScxmlEditor::PluginInterface::GraphicsScene *scene)
{
    m_graphicsView->setScene(scene);
}

void Magnifier::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter p(this);

    p.setPen(Qt::NoPen);
    p.setBrush(m_gradientBrush);
    p.drawRect(rect());
}
