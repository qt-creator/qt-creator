// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
