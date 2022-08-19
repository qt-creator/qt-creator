// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "dockcomponentsfactory.h"

#include "dockwidgettab.h"
#include "dockareatabbar.h"
#include "dockareatitlebar.h"
#include "dockwidget.h"
#include "dockareawidget.h"

#include <memory>

namespace ADS
{
    static std::unique_ptr<DockComponentsFactory> g_defaultFactory(new DockComponentsFactory());

    DockWidgetTab *DockComponentsFactory::createDockWidgetTab(DockWidget *dockWidget) const
    {
        return new DockWidgetTab(dockWidget);
    }

    DockAreaTabBar *DockComponentsFactory::createDockAreaTabBar(DockAreaWidget *dockArea) const
    {
        return new DockAreaTabBar(dockArea);
    }

    DockAreaTitleBar *DockComponentsFactory::createDockAreaTitleBar(DockAreaWidget *dockArea) const
    {
        return new DockAreaTitleBar(dockArea);
    }

    const DockComponentsFactory *DockComponentsFactory::factory()
    {
        return g_defaultFactory.get();
    }

    void DockComponentsFactory::setFactory(DockComponentsFactory *factory)
    {
        g_defaultFactory.reset(factory);
    }

    void DockComponentsFactory::resetDefaultFactory()
    {
        g_defaultFactory.reset(new DockComponentsFactory());
    }

} // namespace ADS
