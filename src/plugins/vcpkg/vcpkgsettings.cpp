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

#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Vcpkg::Internal {

static VcpkgSettings *projectSettings(Project *project)
{
    const Key key = "VcpkgProjectSettings";
    QVariant v = project->extraData(key);
    if (v.isNull()) {
        v = QVariant::fromValue(new VcpkgSettings(project, true));
        project->setExtraData(key, v);
    }
    return v.value<VcpkgSettings *>();
}

VcpkgSettings *settings(Project *project)
{
    static VcpkgSettings theSettings{nullptr, false};
    if (!project)
        return &theSettings;

    VcpkgSettings *projSettings = projectSettings(project);
    if (projSettings->useGlobalSettings)
        return &theSettings;

    return projSettings;
}

VcpkgSettings::VcpkgSettings(Project *project, bool autoApply)
    : m_project(project)
{
    setSettingsGroup(Constants::Settings::GROUP_ID);
    setAutoApply(autoApply);

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

    if (m_project) {
        // Re-read the settings. Reading in constructor is too early
        connect(m_project, &Project::settingsLoaded, this, [this] { readSettings(); });
    }
}

void VcpkgSettings::readSettings()
{
    if (!m_project) {
        AspectContainer::readSettings();
    } else {
        Store data = storeFromVariant(m_project->namedSettings(Constants::Settings::GENERAL_ID));
        if (data.isEmpty()) {
            useGlobalSettings = true;
            AspectContainer::readSettings();
        } else {
            useGlobalSettings = data.value(Constants::Settings::USE_GLOBAL_SETTINGS, true).toBool();
            fromMap(data);
        }
    }
    setVcpkgRootEnvironmentVariable();
}

void VcpkgSettings::writeSettings() const
{
    if (!m_project) {
        AspectContainer::writeSettings();
    } else {
        Store data;
        toMap(data);
        data.insert(Constants::Settings::USE_GLOBAL_SETTINGS, useGlobalSettings);
        m_project->setNamedSettings(Constants::Settings::GENERAL_ID, variantFromStore(data));
    }
}

void VcpkgSettings::setVcpkgRootEnvironmentVariable()
{
    // Set VCPKG_ROOT environment variable so that auto-setup.cmake would pick it up
    Environment::modifySystemEnvironment({{Constants::ENVVAR_VCPKG_ROOT,
        vcpkgRoot.expandedValue().nativePath()}});
}

class VcpkgSettingsPage : public Core::IOptionsPage
{
public:
    VcpkgSettingsPage()
    {
        setId(Constants::Settings::GENERAL_ID);
        setDisplayName("Vcpkg");
        setCategory(Constants::Settings::CATEGORY);
        setSettingsProvider([] { return settings(nullptr); });
    }
};

static const VcpkgSettingsPage settingsPage;

class VcpkgSettingsWidget : public ProjectSettingsWidget
{
public:
    explicit VcpkgSettingsWidget(Project *project)
        : m_widget(new QWidget)
        , m_displayedSettings(project, true)
    {
        setGlobalSettingsId(Constants::Settings::GENERAL_ID);

        // Construct the widget layout from the aspect container
        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        if (auto layouter = m_displayedSettings.layouter())
            layouter().attachTo(m_widget);
        layout->addWidget(m_widget);

        setUseGlobalSettings(m_displayedSettings.useGlobalSettings);
        m_widget->setEnabled(!useGlobalSettings());

        if (project) {
            VcpkgSettings *projSettings = projectSettings(project);

            connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
                    this, [this, projSettings](bool useGlobal) {
                m_widget->setEnabled(!useGlobal);
                m_displayedSettings.useGlobalSettings = useGlobal;
                m_displayedSettings.copyFrom(useGlobal ? *settings(nullptr) : *projSettings);

                projSettings->useGlobalSettings = useGlobal;
                projSettings->writeSettings();
                projSettings->setVcpkgRootEnvironmentVariable();
            });

            // React on Global settings changes
            connect(settings(nullptr), &AspectContainer::changed, this, [this] {
                if (m_displayedSettings.useGlobalSettings)
                    m_displayedSettings.copyFrom(*settings(nullptr));
            });

            // Reflect changes to the project settings in the displayed settings
            connect(projSettings, &AspectContainer::changed, this, [this, projSettings] {
                if (!m_displayedSettings.useGlobalSettings)
                    m_displayedSettings.copyFrom(*projSettings);
            });

            // React on displayed settings changes in the project settings
            connect(&m_displayedSettings, &AspectContainer::changed, this, [this, projSettings] {
                if (!m_displayedSettings.useGlobalSettings) {
                    projSettings->copyFrom(m_displayedSettings);
                    projSettings->writeSettings();
                    projSettings->setVcpkgRootEnvironmentVariable();
                }
            });
        }
    }

    QWidget *m_widget = nullptr;
    VcpkgSettings m_displayedSettings;
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
