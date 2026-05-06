// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrsettings.h"

#include "zephyrconstants.h"
#include "zephyrtr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Zephyr::Internal {

ZephyrSettings::ZephyrSettings()
{
    setSettingsGroup("Zephyr");
    setAutoApply(false);

    westFilePath.setSettingsKey("WestFilePath");
    westFilePath.setLabelText(Tr::tr("west executable:"));
    westFilePath.setExpectedKind(PathChooser::ExistingCommand);
    westFilePath.setDefaultPathValue(FilePath::fromUserInput("west"));
    westFilePath.setPlaceHolderText(Tr::tr("west"));

    workspaceDir.setSettingsKey("WorkspaceDir");
    workspaceDir.setLabelText(Tr::tr("Workspace directory:"));
    workspaceDir.setExpectedKind(PathChooser::ExistingDirectory);
    workspaceDir.setPlaceHolderText(Tr::tr("Directory containing .west/"));
    const FilePath zephyrBase =
        FilePath::fromUserInput(qtcEnvironmentVariable("ZEPHYR_BASE"));
    if (zephyrBase.isDir())
        workspaceDir.setDefaultPathValue(zephyrBase.parentDir());

    qmlProjectExporterFilePath.setSettingsKey("QmlProjectExporterFilePath");
    qmlProjectExporterFilePath.setLabelText(Tr::tr("qmlprojectexporter:"));
    qmlProjectExporterFilePath.setExpectedKind(PathChooser::ExistingCommand);
    qmlProjectExporterFilePath.setPlaceHolderText(Tr::tr("optional, for Qt for MCUs projects"));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("West Build Tool")),
                Form {
                    westFilePath, br,
                    workspaceDir, br,
                    qmlProjectExporterFilePath,
                },
            },
            st,
        };
    });

    readSettings();
}

ZephyrSettings &settings()
{
    static ZephyrSettings theSettings;
    return theSettings;
}

class ZephyrSettingsPage : public Core::IOptionsPage
{
public:
    ZephyrSettingsPage()
    {
        setId(Constants::ZEPHYR_SETTINGS_ID);
        setDisplayName(Tr::tr("Zephyr"));
        setCategory(ProjectExplorer::Constants::SDK_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

const ZephyrSettingsPage settingsPage;

} // namespace Zephyr::Internal
