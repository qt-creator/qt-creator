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

#pragma once

#include "widgethostconstants.h"

#include <QWidget>
#include <QPoint>

namespace SharedTools {
namespace Internal {

class SizeHandleRect : public QWidget
{
    Q_OBJECT
public:
    enum Direction { LeftTop, Top, RightTop, Right, RightBottom, Bottom, LeftBottom, Left };

    SizeHandleRect(QWidget *parent, Direction d, QWidget *resizable);

    Direction dir() const  { return m_dir; }
    void updateCursor();
    void setState(SelectionHandleState st);

signals:

    void mouseButtonReleased(const QRect &, const QRect &);

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void tryResize(const QSize &delta);

private:
    const Direction m_dir;
    QPoint m_startPos;
    QPoint m_curPos;
    QSize m_startSize;
    QSize m_curSize;
    QWidget *m_resizable;
    SelectionHandleState m_state;
};

}
} // namespace SharedTools
