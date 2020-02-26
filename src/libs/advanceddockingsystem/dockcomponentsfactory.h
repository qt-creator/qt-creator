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

namespace ADS {

class DockWidgetTab;
class DockAreaTitleBar;
class DockAreaTabBar;
class DockAreaWidget;
class DockWidget;

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
