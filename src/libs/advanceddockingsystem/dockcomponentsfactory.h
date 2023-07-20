// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"

namespace ADS {

class DockWidgetTab;
class DockAreaTitleBar;
class DockAreaTabBar;
class DockAreaWidget;
class DockWidget;
class AutoHideTab;

/**
 * Factory for creation of certain GUI elements for the docking framework.
 * A default unique instance provided by DockComponentsFactory is used for
 * creation of all supported components. To inject your custom components,
 * you can create your own derived dock components factory and register
 * it via setDefaultFactory() function.
 * \code
 * CDockComponentsFactory::setDefaultFactory(new MyComponentsFactory()));
 * \endcode
 */
class ADS_EXPORT DockComponentsFactory
{
public:
    /**
     * Force virtual destructor
     */
    virtual ~DockComponentsFactory() {}

    /**
     * This default implementation just creates a dock widget tab with
     * new DockWidgetTab(dockWidget).
     */
    virtual DockWidgetTab *createDockWidgetTab(DockWidget *dockWidget) const;

    /**
     * This default implementation just creates a dock widget side tab with
     * new CDockWidgetTab(DockWidget).
     */
    virtual AutoHideTab *createDockWidgetSideTab(DockWidget *dockWidget) const;

    /**
     * This default implementation just creates a dock area tab bar with
     * new DockAreaTabBar(dockArea).
     */
    virtual DockAreaTabBar *createDockAreaTabBar(DockAreaWidget *dockArea) const;

    /**
     * This default implementation just creates a dock area title bar with
     * new DockAreaTitleBar(dockArea).
     */
    virtual DockAreaTitleBar *createDockAreaTitleBar(DockAreaWidget *dockArea) const;

    /**
     * Returns the default components factory
     */
    static const DockComponentsFactory *factory();

    /**
     * Sets a new default factory for creation of GUI elements.
     * This function takes ownership of the given Factory.
     */
    static void setFactory(DockComponentsFactory* factory);

    /**
     * Resets the current factory to the
     */
    static void resetDefaultFactory();
};

/**
 * Convenience function to ease factory instance access
 */
inline const DockComponentsFactory *componentsFactory()
{
    return DockComponentsFactory::factory();
}

} // namespace ADS
