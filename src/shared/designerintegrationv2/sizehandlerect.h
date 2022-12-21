// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
