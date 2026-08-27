// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevicefactory.h"
#include "desktopdevice.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectexplorertr.h>

#include <coreplugin/coreicons.h>

#include <utils/icon.h>

#include <QApplication>
#include <QStyle>

namespace ProjectExplorer::Internal {

DesktopDeviceFactory::DesktopDeviceFactory()
    : IDeviceFactory(Constants::DESKTOP_DEVICE_TYPE)
{
    setConstructionFunction([] { return IDevice::Ptr(new DesktopDevice); });
    setDisplayName(Tr::tr("Desktop"));
    const bool flatSideBarIcons = Utils::creatorTheme()->flag(Utils::Theme::FlatSideBarIcons);
    const QIcon standardIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    setIcon(flatSideBarIcons ? Core::Icons::DESKTOP_DEVICE_SMALL.icon() : standardIcon);
    setTargetSelectorIcon(flatSideBarIcons ? Icons::DESKTOP_DEVICE.icon() : standardIcon);
    setExecutionTypeId(Constants::STDPROCESS_EXECUTION_TYPE_ID);
}

} // namespace ProjectExplorer::Internal
