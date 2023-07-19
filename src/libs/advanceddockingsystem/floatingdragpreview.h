// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

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

    /**
     * Cleanup auto hide container if the dragged widget has one.
     */
    void cleanupAutoHideContainerWidget(DockWidgetArea containerDropArea);

signals:
    /**
     * This signal is emitted, if dragging has been canceled by escape key
     * or by active application switching via task manager
     */
    void draggingCanceled();
}; // class FloatingDragPreview

} // namespace ADS
