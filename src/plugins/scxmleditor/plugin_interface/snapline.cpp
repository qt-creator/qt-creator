// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "snapline.h"
#include <QPen>

using namespace ScxmlEditor::PluginInterface;

SnapLine::SnapLine(QGraphicsItem *parent)
    : QGraphicsLineItem(parent)
{
    QPen pen;
    pen.setBrush(QColor(0x22, 0xcc, 0x22));
    pen.setStyle(Qt::DashLine);
    setPen(pen);
    setZValue(502);

    m_visibilityTimer.setInterval(1000);
    m_visibilityTimer.setSingleShot(true);
    QObject::connect(&m_visibilityTimer, &QTimer::timeout, this, &SnapLine::hideLine);

    hideLine();
}

void SnapLine::show(qreal x1, qreal y1, qreal x2, qreal y2)
{
    setLine(x1, y1, x2, y2);
    setVisible(true);
    m_visibilityTimer.start();
}

void SnapLine::hideLine()
{
    setVisible(false);
}
