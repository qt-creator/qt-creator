// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicesettingspage.h"

#include "devicesettingswidget.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"

#include <QCoreApplication>

namespace ProjectExplorer {
namespace Internal {

DeviceSettingsPage::DeviceSettingsPage()
{
    setId(Constants::DEVICE_SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Devices"));
    setCategory(Constants::DEVICE_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Devices"));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_devices.png");
    setWidgetCreator([] { return new DeviceSettingsWidget; });
}

} // namespace Internal
} // namespace ProjectExplorer
