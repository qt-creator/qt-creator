// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcpkgsettings.h"

#include "vcpkgconstants.h"
#include "vcpkgtr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QToolButton>

using namespace Utils;

namespace Vcpkg::Internal {

VcpkgSettings &settings()
{
    static VcpkgSettings theSettings;
    return theSettings;
}

VcpkgSettings::VcpkgSettings()
{
    setSettingsGroup("Vcpkg");
    setAutoApply(false);

    vcpkgRoot.setSettingsKey("VcpkgRoot");
    vcpkgRoot.setExpectedKind(PathChooser::ExistingDirectory);
    FilePath defaultPath = Environment::systemEnvironment().searchInPath(Constants::VCPKG_COMMAND)
                               .parentDir();
    if (!defaultPath.isDir())
        defaultPath = FilePath::fromUserInput(qtcEnvironmentVariable(Constants::ENVVAR_VCPKG_ROOT));
    if (defaultPath.isDir())
        vcpkgRoot.setDefaultValue(defaultPath.toUserOutput());

    connect(this, &AspectContainer::applied, this, &VcpkgSettings::setVcpkgRootEnvironmentVariable);

    setLayouter([this] {
        using namespace Layouting;
        auto websiteButton = new QToolButton;
        websiteButton->setIcon(Icons::ONLINE.icon());
        websiteButton->setToolTip(Constants::WEBSITE_URL);

        connect(websiteButton, &QAbstractButton::clicked, [] {
            QDesktopServices::openUrl(QUrl::fromUserInput(Constants::WEBSITE_URL));
        });

        // clang-format off
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("Vcpkg installation")),
                Form {
                    PathChooser::label(),
                    Span { 2, Row { vcpkgRoot, websiteButton } },
                },
            },
            st,
        };
        // clang-format on
    });

    readSettings();
}

void VcpkgSettings::setVcpkgRootEnvironmentVariable()
{
    // Set VCPKG_ROOT environment variable so that auto-setup.cmake would pick it up
    Environment::modifySystemEnvironment({{Constants::ENVVAR_VCPKG_ROOT, vcpkgRoot.value()}});
}

class VcpkgSettingsPage : public Core::IOptionsPage
{
public:
    VcpkgSettingsPage()
    {
        setId(Constants::TOOLSSETTINGSPAGE_ID);
        setDisplayName("Vcpkg");
        setCategory(CMakeProjectManager::Constants::Settings::CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

static const VcpkgSettingsPage settingsPage;

} // Vcpkg::Internal
