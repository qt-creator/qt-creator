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

#include "colorpickertool.h"
#include "qdeclarativeviewinspector.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QRectF>
#include <QRgb>
#include <QImage>
#include <QApplication>
#include <QPalette>

namespace QmlJSDebugger {

ColorPickerTool::ColorPickerTool(QDeclarativeViewInspector *view) :
    AbstractLiveEditTool(view)
{
    m_selectedColor.setRgb(0,0,0);
}

ColorPickerTool::~ColorPickerTool()
{

}

void ColorPickerTool::mousePressEvent(QMouseEvent * /*event*/)
{
}

void ColorPickerTool::mouseMoveEvent(QMouseEvent *event)
{
    pickColor(event->pos());
}

void ColorPickerTool::mouseReleaseEvent(QMouseEvent *event)
{
    pickColor(event->pos());
}

void ColorPickerTool::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{
}


void ColorPickerTool::hoverMoveEvent(QMouseEvent * /*event*/)
{
}

void ColorPickerTool::keyPressEvent(QKeyEvent * /*event*/)
{
}

void ColorPickerTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{
}
void ColorPickerTool::wheelEvent(QWheelEvent * /*event*/)
{
}

void ColorPickerTool::itemsAboutToRemoved(const QList<QGraphicsItem*> &/*itemList*/)
{
}

void ColorPickerTool::clear()
{
    view()->setCursor(Qt::CrossCursor);
}

void ColorPickerTool::selectedItemsChanged(const QList<QGraphicsItem*> &/*itemList*/)
{
}

void ColorPickerTool::pickColor(const QPoint &pos)
{
    QRgb fillColor = view()->backgroundBrush().color().rgb();
    if (view()->backgroundBrush().style() == Qt::NoBrush)
        fillColor = view()->palette().color(QPalette::Base).rgb();

    QRectF target(0,0, 1, 1);
    QRect source(pos.x(), pos.y(), 1, 1);
    QImage img(1, 1, QImage::Format_ARGB32);
    img.fill(fillColor);
    QPainter painter(&img);
    view()->render(&painter, target, source);
    m_selectedColor = QColor::fromRgb(img.pixel(0, 0));

    emit selectedColorChanged(m_selectedColor);
}

} // namespace QmlJSDebugger
