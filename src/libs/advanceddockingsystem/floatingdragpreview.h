/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "floatingdockcontainer.h"

#include <QWidget>

namespace ADS {

class DockWidget;
class DockAreaWidget;
class FloatingDragPreviewPrivate;

/**
 * A floating overlay is a temporary floating widget that is just used to
 * indicate the floating widget movement.
 * This widget is used as a placeholder for drag operations for non-opaque
 * docking
 */
class FloatingDragPreview : public QWidget, public AbstractFloatingWidget
{
    Q_OBJECT
private:
    FloatingDragPreviewPrivate *d;
    friend class FloatingDragPreviewPrivate;

    /**
     * Cancel non opaque undocking if application becomes inactive
     */
    void onApplicationStateChanged(Qt::ApplicationState state);

protected:
    /**
     * Cares about painting the
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * The content is a DockArea or a DockWidget
     */
    FloatingDragPreview(QWidget *content, QWidget *parent);

public:
    using Super = QWidget;

    /**
     * Creates an instance for undocking the DockWidget in Content parameter
     */
    FloatingDragPreview(DockWidget *content);

    /**
     * Creates an instance for undocking the DockArea given in Content
     * parameters
     */
    FloatingDragPreview(DockAreaWidget *content);

    /**
     * Delete private data
     */
    ~FloatingDragPreview() override;

    /**
     * We filter the events of the assigned content widget to receive
     * escape key presses for canceling the drag operation
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

public: // implements AbstractFloatingWidget
    void startFloating(const QPoint &dragStartMousePos,
                       const QSize &size,
                       eDragState dragState,
                       QWidget *mouseEventHandler) override;

    /**
     * Moves the widget to a new position relative to the position given when
     * startFloating() was called
     */
    void moveFloating() override;

    /**
     * Finishes dragging.
     * Hides the dock overlays and executes the real undocking and docking
     * of the assigned Content widget
     */
    void finishDragging() override;

signals:
    /**
     * This signal is emitted, if dragging has been canceled by escape key
     * or by active application switching via task manager
     */
    void draggingCanceled();
}; // class FloatingDragPreview

} // namespace ADS
