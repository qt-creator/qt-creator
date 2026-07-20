// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyossettings.h"

#include "harmonyosconfigurations.h"
#include "harmonyosconstants.h"
#include "harmonyossdk.h"
#include "harmonyostr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QLabel>
#include <QPushButton>

using namespace Utils;

namespace HarmonyOs::Internal {

HarmonyOsSettings &settings()
{
    static HarmonyOsSettings theSettings;
    return theSettings;
}

HarmonyOsSettings::HarmonyOsSettings()
{
    setSettingsGroup("HarmonyOSConfiguration");
    setAutoApply(false);

    sdkLocation.setSettingsKey("SdkLocation");
    sdkLocation.setExpectedKind(PathChooser::ExistingDirectory);
    sdkLocation.setLabelText(Tr::tr("HarmonyOS SDK location:"));

    automaticKitCreation.setSettingsKey("AutomaticKitCreation");
    automaticKitCreation.setLabelText(Tr::tr("Create kits automatically"));
    automaticKitCreation.setDefaultValue(true);

    connect(this, &AspectContainer::applied, this, [] { applyConfig(); });

    setLayouter([this] {
        auto instruction = new QLabel(
            Tr::tr("Select the installation directory of DevEco Studio or of the HarmonyOS "
                   "command-line tools. It must contain the OpenHarmony native SDK (with the "
                   "\"llvm\" and \"sysroot\" folders) used to build for HarmonyOS."));
        instruction->setWordWrap(true);

        auto autodetectButton = new QPushButton(Tr::tr("Auto-detect"));

        auto status = new InfoLabel;
        status->setElideMode(Qt::ElideNone);
        status->setWordWrap(true);

        using namespace Layouting;
        Column column {
            Group {
                title(Tr::tr("HarmonyOS SDK")),
                Column {
                    instruction,
                    Row { sdkLocation, autodetectButton },
                    status,
                    automaticKitCreation,
                },
            },
            st,
        };

        // The path chooser widget only exists after the layout above was built.
        const auto updateStatus = [this, status] {
            const FilePath sdkRoot = sdkLocation.pathChooser()->filePath();
            if (sdkRoot.isEmpty()) {
                status->setType(InfoLabelType::None);
                status->setText({});
                return;
            }
            if (Sdk::isValidSdk(sdkRoot)) {
                status->setType(InfoLabelType::Ok);
                const bool hasHvigor = !Sdk::hvigorBinPath(sdkRoot).isEmpty();
                status->setText(hasHvigor
                    ? Tr::tr("A HarmonyOS SDK with the hvigor build tool was found.")
                    : Tr::tr("A HarmonyOS SDK was found, but the hvigor build tool is missing."));
            } else {
                status->setType(InfoLabelType::NotOk);
                status->setText(Tr::tr("No HarmonyOS native SDK was found in this directory."));
            }
        };

        connect(sdkLocation.pathChooser(), &PathChooser::textChanged, this, updateStatus);
        connect(autodetectButton, &QPushButton::clicked, this, [this, status, updateStatus] {
            const FilePath detected = Sdk::detectDevEcoSdk();
            if (detected.isEmpty()) {
                status->setType(InfoLabelType::Warning);
                status->setText(Tr::tr("Could not find an installed DevEco Studio SDK."));
                return;
            }
            sdkLocation.pathChooser()->setFilePath(detected);
            updateStatus();
        });

        updateStatus();

        return column;
    });

    readSettings();
}

// HarmonyOsSettingsPage

class HarmonyOsSettingsPage final : public Core::IOptionsPage
{
public:
    HarmonyOsSettingsPage()
    {
        setId(Constants::HARMONYOS_SETTINGS_ID);
        setDisplayName(Tr::tr("HarmonyOS"));
        setCategory(ProjectExplorer::Constants::SDK_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

void setupHarmonyOsSettingsPage()
{
    static HarmonyOsSettingsPage theHarmonyOsSettingsPage;
}

} // namespace HarmonyOs::Internal
