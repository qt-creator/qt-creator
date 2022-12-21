// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conansettings.h"

#include <utils/hostosinfo.h>

using namespace Utils;

namespace ConanPackageManager {
namespace Internal {

ConanSettings::ConanSettings()
{
    setSettingsGroup("ConanSettings");
    setAutoApply(false);

    registerAspect(&conanFilePath);
    conanFilePath.setSettingsKey("ConanFilePath");
    conanFilePath.setDisplayStyle(StringAspect::PathChooserDisplay);
    conanFilePath.setExpectedKind(PathChooser::ExistingCommand);
    conanFilePath.setDefaultValue(HostOsInfo::withExecutableSuffix("conan"));
}

} // Internal
} // ConanPackageManager
