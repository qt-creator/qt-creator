// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"

#include <QFrame>

namespace ADS {

class ResizeHandlePrivate;

/**
 * Resize handle for resizing its parent widget
 */
class ADS_EXPORT ResizeHandle : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY(ResizeHandle)
    Q_PROPERTY(bool opaqueResize READ opaqueResize WRITE setOpaqueResize NOTIFY opaqueResizeChanged)

private:
    ResizeHandlePrivate *d; ///< private data (pimpl)
    friend class ResizeHandlePrivate;

protected:
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

public:
    using Super = QFrame;

    /**
     * Default Constructor
     */
    ResizeHandle(Qt::Edge handlePosition, QWidget *parent);

    /**
     * Virtual Destructor
     */
    virtual ~ResizeHandle();

    /**
     * Sets the handle position
     */
    void setHandlePosition(Qt::Edge handlePosition);

    /**
     * Returns the handle position
     */
    Qt::Edge handlePostion() const;

    /**
     * Returns the orientation of this resize handle
     */
    Qt::Orientation orientation() const;

    /**
     * Returns the size hint
     */
    QSize sizeHint() const override;

    /**
     * Returns true, if resizing is active
     */
    bool isResizing() const;

    /**
     * Sets the minimum size for the widget that is going to be resized.
     * The resize handle will not resize the target widget to a size smaller
     * than this value
     */
    void setMinResizeSize(int minSize);

    /**
     * Sets the maximum size for the widget that is going to be resized
     * The resize handle will not resize the target widget to a size bigger
     * than this value
     */
    void setMaxResizeSize(int maxSize);

    /**
     * Enable / disable opaque resizing
     */
    void setOpaqueResize(bool opaque = true);

    /**
     * Returns true if widgets are resized dynamically (opaquely) while
     * interactively moving the resize handle. Otherwise returns false.
     */
    bool opaqueResize() const;

signals:
    void opaqueResizeChanged();
}; // class ResizeHandle

} // namespace ADS
