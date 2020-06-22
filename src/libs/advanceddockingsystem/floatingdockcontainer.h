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

#include "ads_globals.h"

#include <QDockWidget>
#include <QRubberBand>

#ifdef Q_OS_LINUX
using FloatingWidgetBaseType = QDockWidget;
#else
using FloatingWidgetBaseType = QWidget;
#endif

namespace ADS {

class FloatingDockContainerPrivate;
class DockManager;
class DockManagerPrivate;
class DockAreaWidget;
class DockContainerWidget;
class DockWidget;
class DockManager;
class DockAreaTabBar;
class DockWidgetTab;
class DockWidgetTabPrivate;
class DockAreaTitleBar;
class DockAreaTitleBarPrivate;
class FloatingWidgetTitleBar;
class DockingStateReader;

/**
 * Pure virtual interface for floating widgets.
 * This interface is used for opaque and non-opaque undocking. If opaque
 * undocking is used, the a real FloatingDockContainer widget will be created
 */
class AbstractFloatingWidget
{
public:
    virtual ~AbstractFloatingWidget() = 0;
    /**
     * Starts floating.
     * This function should get called typically from a mouse press event
     * handler
     */
    virtual void startFloating(const QPoint &dragStartMousePos,
                               const QSize &size,
                               eDragState dragState,
                               QWidget *mouseEventHandler)
        = 0;

    /**
     * Moves the widget to a new position relative to the position given when
     * startFloating() was called.
     * This function should be called from a mouse mouve event handler to
     * move the floating widget on mouse move events.
     */
    virtual void moveFloating() = 0;

    /**
     * Tells the widget that to finish dragging if the mouse is released.
     * This function should be called from a mouse release event handler
     * to finish the dragging
     */
    virtual void finishDragging() = 0;
};

/**
 * This implements a floating widget that is a dock container that accepts
 * docking of dock widgets like the main window and that can be docked into
 * another dock container.
 * Every floating window of the docking system is a FloatingDockContainer.
 */
class ADS_EXPORT FloatingDockContainer : public FloatingWidgetBaseType,
                                         public AbstractFloatingWidget
{
    Q_OBJECT
private:
    FloatingDockContainerPrivate *d; ///< private data (pimpl)
    friend class FloatingDockContainerPrivate;
    friend class DockManager;
    friend class DockManagerPrivate;
    friend class DockAreaTabBar;
    friend class DockWidgetTabPrivate;
    friend class DockWidgetTab;
    friend class DockAreaTitleBar;
    friend class DockAreaTitleBarPrivate;
    friend class DockWidget;
    friend class DockAreaWidget;
    friend class FloatingWidgetTitleBar;

    void onDockAreasAddedOrRemoved();
    void onDockAreaCurrentChanged(int Index);

protected:
    /**
     * Starts floating at the given global position.
     * Use moveToGlobalPos() to move the widget to a new position
     * depending on the start position given in Pos parameter
     */
    void startFloating(const QPoint &dragStartMousePos,
                       const QSize &size,
                       eDragState dragState,
                       QWidget *mouseEventHandler) override;

    /**
     * Call this function to start dragging the floating widget
     */
    void startDragging(const QPoint &dragStartMousePos,
                       const QSize &size,
                       QWidget *mouseEventHandler)
    {
        startFloating(dragStartMousePos, size, DraggingFloatingWidget, mouseEventHandler);
    }

    /**
     * Call this function if you explicitly want to signal that dragging has
     * finished
     */
    void finishDragging() override;

    /**
     * Call this function if you just want to initialize the position
     * and size of the floating widget
     */
    void initFloatingGeometry(const QPoint &dragStartMousePos, const QSize &size)
    {
        startFloating(dragStartMousePos, size, DraggingInactive, nullptr);
    }

    /**
     * Moves the widget to a new position relative to the position given when
     * startFloating() was called
     */
    void moveFloating() override;

    /**
     * Restores the state from given stream.
     * If Testing is true, the function only parses the data from the given
     * stream but does not restore anything. You can use this check for
     * faulty files before you start restoring the state
     */
    bool restoreState(DockingStateReader &stream, bool testing);

    /**
     * Call this function to update the window title
     */
    void updateWindowTitle();

protected: // reimplements QWidget
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

#ifdef Q_OS_MACOS
    virtual bool event(QEvent *event) override;
    virtual void moveEvent(QMoveEvent *event) override;
#endif

#ifdef Q_OS_WIN
    /**
     * Native event filter for handling WM_MOVING messages on Windows
     */
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

public:
    using Super = QWidget;

    /**
     * Create empty floating widget - required for restore state
     */
    FloatingDockContainer(DockManager *dockManager);

    /**
     * Create floating widget with the given dock area
     */
    FloatingDockContainer(DockAreaWidget *dockArea);

    /**
     * Create floating widget with the given dock widget
     */
    FloatingDockContainer(DockWidget *dockWidget);

    /**
     * Virtual Destructor
     */
    ~FloatingDockContainer() override;

    /**
     * Access function for the internal dock container
     */
    DockContainerWidget *dockContainer() const;

    /**
     * This function returns true, if it can be closed.
     * It can be closed, if all dock widgets in all dock areas can be closed
     */
    bool isClosable() const;

    /**
     * This function returns true, if this floating widget has only one single
     * visible dock widget in a single visible dock area.
     * The single dock widget is a real top level floating widget because no
     * other widgets are docked.
     */
    bool hasTopLevelDockWidget() const;

    /**
     * This function returns the first dock widget in the first dock area.
     * If the function hasSingleDockWidget() returns true, then this function
     * returns this single dock widget.
     */
    DockWidget *topLevelDockWidget() const;

    /**
     * This function returns a list of all dock widget in this floating widget.
     * This is a simple convenience function that simply calls the dockWidgets()
     * function of the internal container widget.
     */
    QList<DockWidget *> dockWidgets() const;
}; // class FloatingDockContainer

} // namespace ADS
