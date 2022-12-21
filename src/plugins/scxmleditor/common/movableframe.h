// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>

namespace ScxmlEditor {

namespace Common {

/**
 * @brief The MovableFrame class is the base class for movable frames inside the parent widget.
 *
 * For example Navigator and Warnings are the movable frames.
 */
class MovableFrame : public QFrame
{
    Q_OBJECT

public:
    explicit MovableFrame(QWidget *parent = nullptr);

signals:
    void hideFrame();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void checkCursor(const QPoint &p);

    QPoint m_startPoint;
    bool m_mouseDown = false;
};

} // namespace Common
} // namespace ScxmlEditor
