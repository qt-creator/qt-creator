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

#include "dragshapebutton.h"
#include "baseitem.h"
#include <QDrag>
#include <QGuiApplication>
#include <QMimeData>
#include <QMouseEvent>

using namespace ScxmlEditor::Common;

DragShapeButton::DragShapeButton(QWidget *parent)
    : QToolButton(parent)
{
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setMinimumSize(75, 75);
    setMaximumSize(75, 75);
    setIconSize(QSize(45, 45));
    QFont f = font();
    f.setPointSize(8);
    setFont(f);
}

void DragShapeButton::setShapeInfo(int groupIndex, int shapeIndex)
{
    m_groupIndex = groupIndex;
    m_shapeIndex = shapeIndex;
}

void DragShapeButton::mousePressEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    auto drag = new QDrag(this);
    auto mimeData = new QMimeData;
    mimeData->setData("dragType", "Shape");
    mimeData->setData("groupIndex", QString::number(m_groupIndex).toLatin1());
    mimeData->setData("shapeIndex", QString::number(m_shapeIndex).toLatin1());
    drag->setMimeData(mimeData);
    drag->setPixmap(icon().pixmap(iconSize()));

    drag->exec();
}
