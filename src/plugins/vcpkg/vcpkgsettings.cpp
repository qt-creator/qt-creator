// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcpkgsettings.h"

#include "vcpkgconstants.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QToolButton>

namespace Vcpkg::Internal {

static VcpkgSettings *theSettings = nullptr;

VcpkgSettings &settings()
{
    return *theSettings;
}

VcpkgSettings::VcpkgSettings()
{
    theSettings = this;

    setSettingsGroup("Vcpkg");
    setId(Constants::TOOLSSETTINGSPAGE_ID);
    setDisplayName("Vcpkg");
    setCategory(CMakeProjectManager::Constants::Settings::CATEGORY);

    vcpkgRoot.setSettingsKey("VcpkgRoot");
    vcpkgRoot.setExpectedKind(Utils::PathChooser::ExistingDirectory);
    vcpkgRoot.setDefaultValue(Utils::qtcEnvironmentVariable(Constants::ENVVAR_VCPKG_ROOT));

    setLayouter([this] {
        using namespace Layouting;
        auto websiteButton = new QToolButton;
        websiteButton->setIcon(Utils::Icons::ONLINE.icon());
        websiteButton->setToolTip(Constants::WEBSITE_URL);

        connect(websiteButton, &QAbstractButton::clicked, [] {
            QDesktopServices::openUrl(QUrl::fromUserInput(Constants::WEBSITE_URL));
        });

        // clang-format off
        using namespace Layouting;
        return Column {
            Group {
                title(tr("Vcpkg installation")),
                Form {
                    Utils::PathChooser::label(),
                    Span{ 2, Row{ vcpkgRoot, websiteButton} },
                },
            },
            st,
        };
        // clang-format on
    });

    readSettings();
}

} // Vcpkg::Internal
