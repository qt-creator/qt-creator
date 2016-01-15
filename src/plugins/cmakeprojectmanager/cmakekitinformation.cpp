/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cmakekitinformation.h"
#include "cmakekitconfigwidget.h"
#include "cmaketoolmanager.h"
#include "cmaketool.h"

#include <utils/qtcassert.h>
#include <projectexplorer/task.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QDebug>

using namespace ProjectExplorer;

namespace CMakeProjectManager {

CMakeKitInformation::CMakeKitInformation()
{
    setObjectName(QLatin1String("CMakeKitInformation"));
    setId(CMakeKitInformation::id());
    setPriority(20000);

    //make sure the default value is set if a selected CMake is removed
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeRemoved,
            [this]() { foreach (Kit *k, KitManager::kits()) fix(k); });

    //make sure the default value is set if a new default CMake is set
    connect(CMakeToolManager::instance(), &CMakeToolManager::defaultCMakeChanged,
            [this]() { foreach (Kit *k, KitManager::kits()) fix(k); });
}

Core::Id CMakeKitInformation::id()
{
    return "CMakeProjectManager.CMakeKitInformation";
}

CMakeTool *CMakeKitInformation::cmakeTool(const Kit *k)
{
    if (!k)
        return 0;

    const QVariant id = k->value(CMakeKitInformation::id());
    return CMakeToolManager::findById(Core::Id::fromSetting(id));
}

void CMakeKitInformation::setCMakeTool(Kit *k, const Core::Id id)
{
    QTC_ASSERT(k, return);

    if (id.isValid()) {
        // Only set cmake tools that are known to the manager
        QTC_ASSERT(CMakeToolManager::findById(id), return);
        k->setValue(CMakeKitInformation::id(), id.toSetting());
    } else {
        //setting a empty Core::Id will reset to the default value
        k->setValue(CMakeKitInformation::id(),defaultValue().toSetting());
    }
}

Core::Id CMakeKitInformation::defaultValue()
{
    CMakeTool *defaultTool = CMakeToolManager::defaultCMakeTool();
    if (defaultTool)
        return defaultTool->id();

    return Core::Id();
}

QVariant CMakeKitInformation::defaultValue(Kit *) const
{
    return defaultValue().toSetting();
}

QList<Task> CMakeKitInformation::validate(const Kit *k) const
{
    Q_UNUSED(k);
    return QList<Task>();
}

void CMakeKitInformation::setup(Kit *k)
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(k);
    if (tool)
        return;

    setCMakeTool(k, defaultValue());
}

void CMakeKitInformation::fix(Kit *k)
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(k);
    if (!tool)
        setup(k);
}

KitInformation::ItemList CMakeKitInformation::toUserOutput(const Kit *k) const
{
    CMakeTool *tool = cmakeTool(k);
    return ItemList() << qMakePair(tr("CMake"), tool == 0 ? tr("Unconfigured") : tool->displayName());
}

KitConfigWidget *CMakeKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::CMakeKitConfigWidget(k, this);
}

} // namespace CMakeProjectManager
