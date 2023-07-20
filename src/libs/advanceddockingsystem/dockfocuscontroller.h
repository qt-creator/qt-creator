// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"
#include "dockmanager.h"

#include <QObject>

namespace ADS {

class DockFocusControllerPrivate;
class DockManager;
class FloatingDockContainer;

/**
 * Manages focus styling of dock widgets and handling of focus changes
 */
class ADS_EXPORT DockFocusController : public QObject
{
    Q_OBJECT

private:
    DockFocusControllerPrivate* d; ///< private data (pimpl)
    friend class DockFocusControllerPrivate;

private:
    void onApplicationFocusChanged(QWidget *old, QWidget *now);
    void onFocusWindowChanged(QWindow *focusWindow);
    void onFocusedDockAreaViewToggled(bool open);
    void onStateRestored();
    void onDockWidgetVisibilityChanged(bool visible);

public:
    /**
     * Default Constructor
     */
    DockFocusController(DockManager *dockManager);

    /**
     * Virtual Destructor
     */
    ~DockFocusController() override;

    /**
     * A container needs to call this function if a widget has been dropped
     * into it
     */
    void notifyWidgetOrAreaRelocation(QWidget *relocatedWidget);

    /**
     * This function is called, if a floating widget has been dropped into
     * an new position.
     * When this function is called, all dock widgets of the FloatingWidget
     * are already inserted into its new position
     */
    void notifyFloatingWidgetDrop(FloatingDockContainer *floatingWidget);

    /**
     * Returns the dock widget that has focus style in the ui or a nullptr if
     * not dock widget is painted focused.
     */
    DockWidget *focusedDockWidget() const;

    /**
     * Notifies the dock focus controller, that a the mouse is pressed or released.
     */
    void setDockWidgetTabPressed(bool value);

    /**
     * Request focus highlighting for the given dock widget assigned to the tab
     * given in Tab parameter
     */
    void setDockWidgetTabFocused(DockWidgetTab *tab);

    /*
     * Request clear focus for a dock widget
     */
    void clearDockWidgetFocus(DockWidget *dockWidget);

    /**
     * Request a focus change to the given dock widget
     */
    void setDockWidgetFocused(DockWidget *focusedNow);
}; // class DockFocusController

} // namespace ADS
