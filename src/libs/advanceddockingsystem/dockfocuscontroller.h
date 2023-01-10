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
    void onFocusedDockAreaViewToggled(bool open);
    void onStateRestored();

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
     * Helper function to set focus depending on the configuration of the
     * FocusStyling flag
     */
    template <class QWidgetPtr>
    static void setWidgetFocus(QWidgetPtr widget)
    {
        if (!DockManager::testConfigFlag(DockManager::FocusHighlighting))
            return;

        widget->setFocus(Qt::OtherFocusReason);
    }

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
     * Request a focus change to the given dock widget
     */
    void setDockWidgetFocused(DockWidget *focusedNow);
}; // class DockFocusController

} // namespace ADS
