// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iossettingspage.h"

#include "iosconstants.h"
#include "iossettingswidget.h"
#include "iostr.h"

#include <projectexplorer/projectexplorerconstants.h>

namespace Ios::Internal {

IosSettingsPage::IosSettingsPage()
{
    setId(Constants::IOS_SETTINGS_ID);
    setDisplayName(Tr::tr("iOS"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new IosSettingsWidget; });
}

} // Ios::Internal
