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
