// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer::Internal {

class WindowsAppSdkSettings : public Utils::AspectContainer
{
    WindowsAppSdkSettings();
    friend WindowsAppSdkSettings &windowsAppSdkSettings();

public:
    Utils::FilePathAspect downloadLocation{this};
    Utils::FilePathAspect nugetLocation{this};
    Utils::FilePathAspect windowsAppSdkLocation{this};
};

WindowsAppSdkSettings &windowsAppSdkSettings();

void setupWindowsAppSdkSettings();

} // namespace ProjectExplorer::Internal
