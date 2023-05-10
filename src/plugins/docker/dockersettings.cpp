// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockersettings.h"

#include "dockerconstants.h"
#include "dockertr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Docker::Internal {

DockerSettings::DockerSettings()
{
    setSettingsGroup(Constants::DOCKER);
    setId(Docker::Constants::DOCKER_SETTINGS_ID);
    setDisplayName(Tr::tr("Docker"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);

    setLayouter([this](QWidget *widget) {
        using namespace Layouting;

        // clang-format off
        Column {
            Group {
                title(Tr::tr("Configuration")),
                Row { dockerBinaryPath }
            },
            st
        }.attachTo(widget);
        // clang-format on
    });

    FilePaths additionalPaths;
    if (HostOsInfo::isWindowsHost())
        additionalPaths.append("C:/Program Files/Docker/Docker/resources/bin");
    else
        additionalPaths.append("/usr/local/bin");

    registerAspect(&dockerBinaryPath);
    dockerBinaryPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    dockerBinaryPath.setExpectedKind(PathChooser::ExistingCommand);
    dockerBinaryPath.setDefaultFilePath(
        FilePath::fromString("docker").searchInPath(additionalPaths));
    dockerBinaryPath.setDisplayName(Tr::tr("Docker CLI"));
    dockerBinaryPath.setHistoryCompleter("Docker.Command.History");
    dockerBinaryPath.setLabelText(Tr::tr("Command:"));
    dockerBinaryPath.setSettingsKey("cli");

    readSettings(Core::ICore::settings());
}

} // Docker::Internal
