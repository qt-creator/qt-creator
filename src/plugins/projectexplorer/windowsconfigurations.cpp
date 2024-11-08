// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsconfigurations.h"

#include "projectexplorerconstants.h"

#include <utils/environment.h>

using namespace Utils;

namespace ProjectExplorer::Internal {

WindowsConfigurations &windowsConfigurations()
{
    static WindowsConfigurations theWindowsConfigurations;
    return theWindowsConfigurations;
}

WindowsConfigurations::WindowsConfigurations()
{
    setSettingsGroup("WindowsConfigurations");

    downloadLocation.setSettingsKey("DownloadLocation");
    nugetLocation.setSettingsKey("NugetLocation");
    windowsAppSdkLocation.setSettingsKey("WindowsAppSDKLocation");

    AspectContainer::readSettings();

    if (windowsAppSdkLocation().isEmpty()) {
        windowsAppSdkLocation.setValue(FilePath::fromUserInput(
            Environment::systemEnvironment().value(Constants::WINDOWS_WINAPPSDK_ROOT_ENV_KEY)));
    }
}

void WindowsConfigurations::applyConfig()
{
    emit aboutToUpdate();
    AspectContainer::writeSettings();
    emit updated();
}

void setupWindowsConfigurations()
{
    (void) windowsConfigurations();
}

} // namespace ProjectExplorer::Internal
