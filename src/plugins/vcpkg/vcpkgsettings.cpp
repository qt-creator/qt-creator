// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcpkgsettings.h"

#include "vcpkgconstants.h"

#include <coreplugin/icore.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QToolButton>

namespace Vcpkg::Internal {

VcpkgSettings::VcpkgSettings()
{
    setSettingsGroup("Vcpkg");

    registerAspect(&vcpkgRoot);
    vcpkgRoot.setSettingsKey("VcpkgRoot");
    vcpkgRoot.setDisplayStyle(Utils::StringAspect::PathChooserDisplay);
    vcpkgRoot.setExpectedKind(Utils::PathChooser::ExistingDirectory);
    vcpkgRoot.setDefaultValue(Utils::qtcEnvironmentVariable(Constants::ENVVAR_VCPKG_ROOT));

    readSettings(Core::ICore::settings());
}

VcpkgSettings *VcpkgSettings::instance()
{
    static VcpkgSettings s;
    return &s;
}

bool VcpkgSettings::vcpkgRootValid() const
{
    return (vcpkgRoot.filePath() / "vcpkg").withExecutableSuffix().isExecutableFile();
}

VcpkgSettingsPage::VcpkgSettingsPage()
{
    setId(Constants::TOOLSSETTINGSPAGE_ID);
    setDisplayName("Vcpkg");
    setCategory(CMakeProjectManager::Constants::Settings::CATEGORY);

    setLayouter([] (QWidget *widget) {
        auto websiteButton = new QToolButton;
        websiteButton->setIcon(Utils::Icons::ONLINE.icon());
        websiteButton->setToolTip(Constants::WEBSITE_URL);

        using namespace Utils::Layouting;
        Column {
            Group {
                title(tr("Vcpkg installation")),
                Form {
                    Utils::PathChooser::label(),
                    Span{ 2, Row{ VcpkgSettings::instance()->vcpkgRoot, websiteButton} },
                },
            },
            st,
        }.attachTo(widget);

        connect(websiteButton, &QAbstractButton::clicked, [] {
            QDesktopServices::openUrl(QUrl::fromUserInput(Constants::WEBSITE_URL));
        });
    });
}

void VcpkgSettingsPage::apply()
{
    VcpkgSettings::instance()->writeSettings(Core::ICore::settings());
}

} // namespace Vcpkg::Internal
