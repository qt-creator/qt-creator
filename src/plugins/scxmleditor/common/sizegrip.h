// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace ScxmlEditor {

namespace Common {

class SizeGrip : public QWidget
{
public:
    SizeGrip(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void leaveEvent(QEvent *e) override;

private:
    void checkCursor(const QPoint &p);

    QPolygon m_pol;
    QPoint m_startPoint;
    QRect m_startRect;
    bool m_mouseDown = false;
};

} // namespace Common
} // namespace ScxmlEditor
