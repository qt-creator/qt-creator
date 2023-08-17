// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conansettings.h"

#include <utils/hostosinfo.h>

using namespace Utils;

namespace Conan::Internal {

ConanSettings &settings()
{
    static ConanSettings theSettings;
    return theSettings;
}

ConanSettings::ConanSettings()
{
    setSettingsGroup("ConanSettings");
    setAutoApply(false);

    conanFilePath.setSettingsKey("ConanFilePath");
    conanFilePath.setExpectedKind(PathChooser::ExistingCommand);
    conanFilePath.setDefaultValue(HostOsInfo::withExecutableSuffix("conan"));

    readSettings();
}

} // Conan::Internal
