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
