// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeproject.h"
#include "cmakespecificsettings.h"

#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectimporter.h>
#include <projectexplorer/projectpanelfactory.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>

#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

CMakeSpecificSettings &settings(Project *project)
{
    static CMakeSpecificSettings theSettings(nullptr, false);
    if (!project)
        return theSettings;

    CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(project);
    if (!cmakeProject || cmakeProject->settings().useGlobalSettings)
        return theSettings;

    return cmakeProject->settings();
}

CMakeSpecificSettings::CMakeSpecificSettings(Project *p, bool autoApply)
    : project(p)
{
    setLayouter([this] {
        using namespace Layouting;
        return Column {
            autorunCMake,
            packageManagerAutoSetup,
            askBeforeReConfigureInitialParams,
            askBeforePresetsReload,
            showSourceSubFolders,
            showAdvancedOptionsByDefault,
            useJunctionsForSourceAndBuildDirectories,
            st
        };
    });

    // TODO: fixup of QTCREATORBUG-26289 , remove in Qt Creator 7 or so
    Core::ICore::settings()->remove("CMakeSpecificSettings/NinjaPath");

    setSettingsGroup(Constants::Settings::GENERAL_ID);
    setAutoApply(autoApply);

    autorunCMake.setSettingsKey("AutorunCMake");
    autorunCMake.setDefaultValue(true);
    autorunCMake.setLabelText(::CMakeProjectManager::Tr::tr("Autorun CMake"));
    autorunCMake.setToolTip(::CMakeProjectManager::Tr::tr(
        "Automatically run CMake after changes to CMake project files."));

    ninjaPath.setSettingsKey("NinjaPath");
    // never save this to the settings:
    ninjaPath.setToSettingsTransformation(
        [](const QVariant &) { return QVariant::fromValue(QString()); });
    ninjaPath.setFromSettingsTransformation([](const QVariant &from) {
        // Sometimes the installer appends the same ninja path to the qtcreator.ini file
        const QString path = from.canConvert<QStringList>() ? from.toStringList().last()
                                                            : from.toString();
        return FilePath::fromUserInput(path).toVariant();
    });

    packageManagerAutoSetup.setSettingsKey("PackageManagerAutoSetup");
    packageManagerAutoSetup.setDefaultValue(true);
    packageManagerAutoSetup.setLabelText(::CMakeProjectManager::Tr::tr("Package manager auto setup"));
    packageManagerAutoSetup.setToolTip(::CMakeProjectManager::Tr::tr("Add the CMAKE_PROJECT_INCLUDE_BEFORE variable "
        "pointing to a CMake script that will install dependencies from the conanfile.txt, "
        "conanfile.py, or vcpkg.json file from the project source directory."));

    askBeforeReConfigureInitialParams.setSettingsKey("AskReConfigureInitialParams");
    askBeforeReConfigureInitialParams.setDefaultValue(true);
    askBeforeReConfigureInitialParams.setLabelText(::CMakeProjectManager::Tr::tr("Ask before re-configuring with "
        "initial parameters"));

    askBeforePresetsReload.setSettingsKey("AskBeforePresetsReload");
    askBeforePresetsReload.setDefaultValue(true);
    askBeforePresetsReload.setLabelText(::CMakeProjectManager::Tr::tr("Ask before reloading CMake Presets"));

    showSourceSubFolders.setSettingsKey("ShowSourceSubFolders");
    showSourceSubFolders.setDefaultValue(true);
    showSourceSubFolders.setLabelText(
                ::CMakeProjectManager::Tr::tr("Show subfolders inside source group folders"));

    showAdvancedOptionsByDefault.setSettingsKey("ShowAdvancedOptionsByDefault");
    showAdvancedOptionsByDefault.setDefaultValue(false);
    showAdvancedOptionsByDefault.setLabelText(
                ::CMakeProjectManager::Tr::tr("Show advanced options by default"));

    useJunctionsForSourceAndBuildDirectories.setSettingsKey(
        "UseJunctionsForSourceAndBuildDirectories");
    useJunctionsForSourceAndBuildDirectories.setDefaultValue(false);
    useJunctionsForSourceAndBuildDirectories.setLabelText(::CMakeProjectManager::Tr::tr(
        "Use junctions for CMake configuration and build operations"));
    useJunctionsForSourceAndBuildDirectories.setVisible(Utils::HostOsInfo().isWindowsHost());
    useJunctionsForSourceAndBuildDirectories.setToolTip(::CMakeProjectManager::Tr::tr(
        "Create and use junctions for the source and build directories to overcome "
        "issues with long paths on Windows.<br><br>"
        "Junctions are stored under <tt>C:\\ProgramData\\QtCreator\\Links</tt> (overridable via "
        "the <tt>QTC_CMAKE_JUNCTIONS_DIR</tt> environment variable).<br><br>"
        "With <tt>QTC_CMAKE_JUNCTIONS_HASH_LENGTH</tt>, you can shorten the MD5 hash key length "
        "to a value smaller than the default length value of 32.<br><br>"
        "Junctions are used for CMake configure, build and install operations."));

    readSettings();

    if (project) {
        // Re-read the settings. Reading in constructor is too early
        connect(project, &Project::settingsLoaded, this, [this] { readSettings(); });

        connect(project->projectImporter(), &ProjectImporter::cmakePresetsUpdated, this, [this] {
            // clear settings first
            Store data;
            project->setNamedSettings(Constants::Settings::GENERAL_ID, variantFromStore(data));

            readSettings();
        });
    }
}

void CMakeSpecificSettings::readSettings()
{
    if (!project) {
        AspectContainer::readSettings();
    } else {
        Store data = storeFromVariant(project->namedSettings(Constants::Settings::GENERAL_ID));
        if (data.isEmpty()) {
            CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(project);
            if (cmakeProject && cmakeProject->presetsData().havePresets
                && cmakeProject->presetsData().vendor) {
                useGlobalSettings = false;
                data = storeFromMap(cmakeProject->presetsData().vendor.value());
                fromMap(data);

                // Write the new loaded CMakePresets settings into .user file
                writeSettings();
            } else {
                useGlobalSettings = true;
                AspectContainer::readSettings();
            }
        } else {
            useGlobalSettings = data.value(Constants::Settings::USE_GLOBAL_SETTINGS, true).toBool();
            fromMap(data);
        }
    }
}

void CMakeSpecificSettings::writeSettings() const
{
    if (!project) {
        AspectContainer::writeSettings();
    } else {
        Store data;
        toMap(data);
        data.insert(Constants::Settings::USE_GLOBAL_SETTINGS, useGlobalSettings);
        project->setNamedSettings(Constants::Settings::GENERAL_ID, variantFromStore(data));
    }
}

class CMakeSpecificSettingsPage final : public Core::IOptionsPage
{
public:
    CMakeSpecificSettingsPage()
    {
        setId(Constants::Settings::GENERAL_ID);
        setDisplayName(::CMakeProjectManager::Tr::tr("General"));
        setDisplayCategory("CMake");
        setCategory(Constants::Settings::CATEGORY);
        setCategoryIconPath(Constants::Icons::SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(nullptr); });
    }
};

const CMakeSpecificSettingsPage settingsPage;

class CMakeProjectSettingsWidget : public ProjectSettingsWidget
{
public:
    explicit CMakeProjectSettingsWidget(Project *project)
        : m_widget(new QWidget)
        , m_project(qobject_cast<CMakeProject *>(project))
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

        if (m_project) {
            connect(
                this, &ProjectSettingsWidget::useGlobalSettingsChanged, this, [this](bool useGlobal) {
                    m_widget->setEnabled(!useGlobal);
                    m_displayedSettings.useGlobalSettings = useGlobal;
                    m_displayedSettings.copyFrom(
                        useGlobal ? settings(nullptr) : m_project->settings());

                    m_project->settings().useGlobalSettings = useGlobal;
                    m_project->settings().writeSettings();
                });

            // React on Global settings changes
            connect(&settings(nullptr), &AspectContainer::changed, this, [this] {
                if (m_displayedSettings.useGlobalSettings)
                    m_displayedSettings.copyFrom(settings(nullptr));
            });

            // Reflect changes to the project settings in the displayed settings
            connect(&m_project->settings(), &AspectContainer::changed, this, [this] {
                if (!m_displayedSettings.useGlobalSettings)
                    m_displayedSettings.copyFrom(m_project->settings());
            });

            // React on project settings changes in the "CMake" project settings
            connect(&m_displayedSettings, &AspectContainer::changed, this, [this] {
                if (!m_displayedSettings.useGlobalSettings) {
                    m_project->settings().copyFrom(m_displayedSettings);
                    m_project->settings().writeSettings();
                }
            });
        } else {
            // Only for CMake projects
            setUseGlobalSettingsCheckBoxEnabled(false);
        }
    }

    QWidget *m_widget = nullptr;
    CMakeProject *m_project = nullptr;
    CMakeSpecificSettings m_displayedSettings;
};

class CMakeProjectSettingsPanelFactory final : public ProjectPanelFactory
{
public:
    CMakeProjectSettingsPanelFactory()
    {
        setPriority(120);
        setDisplayName("CMake");
        setCreateWidgetFunction([](Project *project) {
            return new CMakeProjectSettingsWidget(project);
        });
    }
};

const CMakeProjectSettingsPanelFactory projectSettingsPane;

} // CMakeProjectManager::Internal
