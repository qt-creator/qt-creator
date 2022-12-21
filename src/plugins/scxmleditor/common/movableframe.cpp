// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "movableframe.h"
#include <QMouseEvent>

using namespace ScxmlEditor::Common;

MovableFrame::MovableFrame(QWidget *parent)
    : QFrame(parent)
{
    setContentsMargins(0, 0, 0, 0);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
}

void MovableFrame::mousePressEvent(QMouseEvent *e)
{
    QFrame::mousePressEvent(e);
    m_startPoint = e->pos();
    m_mouseDown = true;
}

void MovableFrame::mouseMoveEvent(QMouseEvent *e)
{
    QFrame::mouseMoveEvent(e);

    if (m_mouseDown) {
        QPoint p = mapToParent(e->pos()) - m_startPoint;
        move(qBound(1, p.x(), parentWidget()->width() - width() - 1), qBound(1, p.y(), parentWidget()->height() - height() - 1));
    }
}

void MovableFrame::mouseReleaseEvent(QMouseEvent *e)
{
    QFrame::mouseReleaseEvent(e);
    m_mouseDown = false;
}
