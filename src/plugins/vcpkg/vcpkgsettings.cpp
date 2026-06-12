// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcpkgsettings.h"

#include "vcpkgconstants.h"
#include "vcpkgtr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectimporter.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettings.h>
#include <projectexplorer/useglobalaspect.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QToolButton>
#include <QVariant>

using namespace ProjectExplorer;
using namespace Utils;

namespace Vcpkg::Internal {

// --- VcpkgSettings -----------------------------------------------------------

VcpkgSettings::VcpkgSettings()
{
    setSettingsGroup(Constants::Settings::GROUP_ID);
    setAutoApply(false);

    vcpkgRoot.setSettingsKey("VcpkgRoot");
    vcpkgRoot.setExpectedKind(PathChooser::ExistingDirectory);
    FilePath defaultPath = FilePath::fromUserInput(
        qtcEnvironmentVariable(Constants::ENVVAR_VCPKG_ROOT));

    if (!defaultPath.isDir())
        defaultPath = Environment::systemEnvironment().searchInPath(Constants::VCPKG_COMMAND).parentDir();

    if (defaultPath.isDir())
        vcpkgRoot.setDefaultPathValue(defaultPath);

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
    setVcpkgRootEnvironmentVariable();
}

void VcpkgSettings::setVcpkgRootEnvironmentVariable()
{
    Environment::modifySystemEnvironment({{Constants::ENVVAR_VCPKG_ROOT,
        vcpkgRoot.expandedValue().nativePath()}});
}

// --- VcpkgProjectSettings ----------------------------------------------------

class VcpkgProjectSettings : public VcpkgSettings
{
public:
    explicit VcpkgProjectSettings(Project *project)
        : m_project(project)
    {
        setAutoApply(true);

        // Load project data (after base constructor loaded global data)
        Store data = storeFromVariant(project->namedSettings(Constants::Settings::GENERAL_ID));
        fromMap(data);
        useGlobalSettings.setValue(data.value(Constants::Settings::USE_GLOBAL_SETTINGS, true).toBool());

        vcpkgRoot.setEnabled(!useGlobalSettings());
        setVcpkgRootEnvironmentVariable();

        // Set up save connections after loading to avoid spurious saves during init
        useGlobalSettings.addOnChanged(this, [this] {
            vcpkgRoot.setEnabled(!useGlobalSettings());
            save();
        });
        vcpkgRoot.addOnChanged(this, [this] {
            if (!useGlobalSettings())
                save();
        });

        // Re-read when project is fully loaded (reading in constructor is too early)
        connect(m_project, &Project::settingsLoaded, this, [this] {
            Store data = storeFromVariant(m_project->namedSettings(Constants::Settings::GENERAL_ID));
            fromMap(data);
            useGlobalSettings.setValue(data.value(Constants::Settings::USE_GLOBAL_SETTINGS, true).toBool());
            setVcpkgRootEnvironmentVariable();
        });
    }

    void save()
    {
        Store data;
        toMap(data);
        data.insert(Constants::Settings::USE_GLOBAL_SETTINGS, useGlobalSettings());
        m_project->setNamedSettings(Constants::Settings::GENERAL_ID, variantFromStore(data));
        setVcpkgRootEnvironmentVariable();
    }

    static Key extraDataKey() { return "VcpkgProjectSettings"; }

    UseGlobalAspect useGlobalSettings{Constants::Settings::GENERAL_ID};

private:
    Project * const m_project;
};

// --- Helpers -----------------------------------------------------------------

static VcpkgProjectSettings *vcpkgProjectSettings(Project *project)
{
    return ProjectExplorer::projectSettings<VcpkgProjectSettings>(project);
}

VcpkgSettings *vcpkgSettingsForProject(Project *project)
{
    static VcpkgSettings theSettings;
    if (!project)
        return &theSettings;
    VcpkgProjectSettings *ps = vcpkgProjectSettings(project);
    if (ps->useGlobalSettings())
        return &theSettings;
    return ps;
}

// --- Settings page -----------------------------------------------------------

class VcpkgSettingsPage : public Core::IOptionsPage
{
public:
    VcpkgSettingsPage()
    {
        setId(Constants::Settings::GENERAL_ID);
        setDisplayName("Vcpkg");
        setCategory(Constants::Settings::CATEGORY);
        setSettingsProvider([] { return vcpkgSettingsForProject(nullptr); });
    }
};

static const VcpkgSettingsPage settingsPage;

// --- Project panel -----------------------------------------------------------

class VcpkgSettingsWidget : public QWidget
{
public:
    explicit VcpkgSettingsWidget(Project *project)
    {
        VcpkgProjectSettings * const ps = vcpkgProjectSettings(project);
        QTC_ASSERT(ps, return);
        using namespace Layouting;
        Column {
            ps->useGlobalSettings,
            *ps,
            noMargin,
        }.attachTo(this);
    }
};

class VcpkgSettingsPanelFactory final : public ProjectPanelFactory
{
public:
    VcpkgSettingsPanelFactory()
    {
        setPriority(120);
        setDisplayName("Vcpkg");
        setCreateWidgetFunction([](Project *project) {
            return new VcpkgSettingsWidget(project);
        });
    }
};

const VcpkgSettingsPanelFactory projectSettingsPane;

} // Vcpkg::Internal
