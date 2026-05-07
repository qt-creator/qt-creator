// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockersettings.h"

#include "dockerconstants.h"
#include "dockertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Docker::Internal {

ContainerToolSettings::ContainerToolSettings(
    Id typeId,
    const QString &scheme,
    const QString &displayType,
    const FilePaths &additionalBinaryPaths)
    : m_typeId(typeId)
    , m_scheme(scheme)
    , m_displayType(displayType)
{
    setAutoApply(false);
    setSettingsGroup(scheme);

    setLayouter([this] {
        using namespace Layouting;
        // clang-format off
        return Column {
            Group {
                title(Tr::tr("Configuration")),
                Row { binaryPath }
            },
            st
        };
        // clang-format on
    });

    const QString toolName = scheme.at(0).toUpper() + scheme.mid(1);

    binaryPath.setExpectedKind(PathChooser::ExistingCommand);
    binaryPath.setDefaultValue(
        FilePath::fromString(scheme).searchInPath(additionalBinaryPaths).toUserOutput());
    binaryPath.setDisplayName(Tr::tr("%1 CLI").arg(displayType));
    binaryPath.setHistoryCompleter((toolName + ".Command.History").toLatin1());
    binaryPath.setLabelText(Tr::tr("Command:"));
    binaryPath.setSettingsKey("cli");

    readSettings();
}

ContainerToolSettings &dockerSettings()
{
    static ContainerToolSettings s{
        Constants::DOCKER_DEVICE_TYPE,
        "docker",
        Tr::tr("Docker"),
        HostOsInfo::isWindowsHost()
            ? FilePaths{FilePath::fromString("C:/Program Files/Docker/Docker/resources/bin")}
            : FilePaths{FilePath::fromString("/usr/local/bin")}
    };
    return s;
}

ContainerToolSettings &podmanSettings()
{
    static ContainerToolSettings s{
        Podman::Constants::PODMAN_DEVICE_TYPE,
        "podman",
        Tr::tr("Podman"),
        HostOsInfo::isWindowsHost()
            ? FilePaths{}
            : FilePaths{FilePath::fromUserInput("~/.local/bin"),
                        FilePath::fromString("/usr/bin")}
    };
    return s;
}

class DockerSettingsPage final : public Core::IOptionsPage
{
public:
    DockerSettingsPage()
    {
        setId(Docker::Constants::DOCKER_SETTINGS_ID);
        setDisplayName(Tr::tr("Docker"));
        setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &dockerSettings(); });
    }
};

const DockerSettingsPage dockerSettingsPage;

class PodmanSettingsPage final : public Core::IOptionsPage
{
public:
    PodmanSettingsPage()
    {
        setId(Podman::Constants::PODMAN_SETTINGS_ID);
        setDisplayName(Tr::tr("Podman"));
        setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &podmanSettings(); });
    }
};

const PodmanSettingsPage podmanSettingsPage;

} // namespace Docker::Internal
