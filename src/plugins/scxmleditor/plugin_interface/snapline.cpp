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
