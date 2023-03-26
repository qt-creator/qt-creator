// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevicefactory.h"
#include "desktopdevice.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectexplorertr.h>

#include <coreplugin/coreicons.h>

#include <utils/icon.h>
#include <utils/qtcassert.h>

#include <QStyle>

namespace ProjectExplorer {
namespace Internal {

DesktopDeviceFactory::DesktopDeviceFactory()
    : IDeviceFactory(Constants::DESKTOP_DEVICE_TYPE)
{
    setConstructionFunction([] { return IDevice::Ptr(new DesktopDevice); });
    setDisplayName(Tr::tr("Desktop"));
    setIcon(Utils::creatorTheme()->flag(Utils::Theme::FlatSideBarIcons)
                ? Utils::Icon::combinedIcon(
                    {Icons::DESKTOP_DEVICE.icon(), Core::Icons::DESKTOP_DEVICE_SMALL.icon()})
                : QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
}

} // namespace Internal
} // namespace ProjectExplorer
