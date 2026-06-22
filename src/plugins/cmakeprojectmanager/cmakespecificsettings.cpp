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
#include <utils/qtcassert.h>

#include <QGuiApplication>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

CMakeSpecificSettings &cmakeSettingsForProject(Project *project)
{
    static CMakeSpecificSettings theSettings(nullptr, false);
    if (!project)
        return theSettings;

    CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(project);
    if (!cmakeProject || cmakeProject->settings().useGlobalSettings())
        return theSettings;

    return cmakeProject->settings();
}

QVariant NinjaPathAspect::fromSettingsValue(const QVariant &savedValue) const
{
   // Sometimes the installer appends the same ninja path to the qtcreator.ini file
   const QString path = savedValue.canConvert<QStringList>()
           ? savedValue.toStringList().last() : savedValue.toString();
   return FilePath::fromUserInput(path).toVariant();
}

QVariant NinjaPathAspect::toSettingsValue(const QVariant &valueToSave) const
{
    // never save this to the settings:
    Q_UNUSED(valueToSave);
    return QVariant::fromValue(QString());
}

CMakeSpecificSettings::CMakeSpecificSettings(Project *p, bool autoApply)
    : project(p)
{
    useGlobalSettings.setSettingsPageId(Constants::Settings::GENERAL_ID);

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            autorunCMake,
            cleanOldOutput,
            packageManagerAutoSetup,
            maintenanceToolDependencyProvider,
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

    configureDetailsExpanded.setSettingsKey("ConfigureDetailsExpanded");
    configureDetailsExpanded.setDefaultValue(true);

    packageManagerAutoSetup.setSettingsKey("PackageManagerAutoSetup");
    packageManagerAutoSetup.setDefaultValue(true);
    packageManagerAutoSetup.setLabelText(::CMakeProjectManager::Tr::tr("Package manager auto setup"));
    packageManagerAutoSetup.setToolTip(
        //: %1 = applicationDisplayName
        ::CMakeProjectManager::Tr::tr(
            "Enables %1 to install dependencies from the conanfile.txt, "
            "conanfile.py, or vcpkg.json file from the project source directory.")
            .arg(QGuiApplication::applicationDisplayName()));

    maintenanceToolDependencyProvider.setSettingsKey("MaintenanceToolDependencyProvider");
    maintenanceToolDependencyProvider.setDefaultValue(true);
    maintenanceToolDependencyProvider.setLabelText(
        ::CMakeProjectManager::Tr::tr("Qt Online Installer dependency provider"));
    maintenanceToolDependencyProvider.setToolTip(
        ::CMakeProjectManager::Tr::tr("Use Qt Online Installer to install missing Qt components."));

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
    useJunctionsForSourceAndBuildDirectories.setVisible(HostOsInfo::isWindowsHost());
    useJunctionsForSourceAndBuildDirectories.setToolTip(::CMakeProjectManager::Tr::tr(
        "Create and use junctions for the source and build directories to overcome "
        "issues with long paths on Windows.<br><br>"
        "Junctions are stored under <tt>C:\\ProgramData\\QtCreator\\Links</tt> (overridable via "
        "the <tt>QTC_CMAKE_JUNCTIONS_DIR</tt> environment variable).<br><br>"
        "With <tt>QTC_CMAKE_JUNCTIONS_HASH_LENGTH</tt>, you can shorten the MD5 hash key length "
        "to a value smaller than the default length value of 32.<br><br>"
        "Junctions are used for CMake configure, build and install operations."));

    cleanOldOutput.setSettingsKey("CleanOldOutput");
    cleanOldOutput.setDefaultValue(true);
    cleanOldOutput.setLabelText(
        ::CMakeProjectManager::Tr::tr("Clear old CMake output on a new run"));

    readSettings();

    if (project) {
        setEnabled(!useGlobalSettings());

        useGlobalSettings.addOnChanged(this, [this] {
            setEnabled(!useGlobalSettings());
            writeSettings();
        });
        addOnChanged(this, [this] {
            if (!useGlobalSettings())
                writeSettings();
        });

        // Re-read the settings. Reading in constructor is too early
        connect(project, &Project::settingsLoaded, this, [this] { readSettings(); });
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
                useGlobalSettings.setValue(false);
                data = storeFromMap(cmakeProject->presetsData().vendor.value());
                fromMap(data);

                // Write the new loaded CMakePresets settings into .user file
                writeSettings();
            } else {
                useGlobalSettings.setValue(true);
                AspectContainer::readSettings();
            }
        } else {
            useGlobalSettings.setValue(data.value(Constants::Settings::USE_GLOBAL_SETTINGS, true).toBool());
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
        data.insert(Constants::Settings::USE_GLOBAL_SETTINGS, useGlobalSettings());
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
        setCategory(Constants::Settings::CATEGORY);
        setSettingsProvider([] { return &cmakeSettingsForProject(nullptr); });
    }
};

const CMakeSpecificSettingsPage settingsPage;

class CMakeProjectSettingsWidget : public QWidget
{
public:
    explicit CMakeProjectSettingsWidget(Project *project)
    {
        auto *cmakeProject = qobject_cast<CMakeProject *>(project);
        QTC_ASSERT(cmakeProject, return);
        CMakeSpecificSettings &ps = cmakeProject->settings();
        using namespace Layouting;
        Column {
            ps.useGlobalSettings,
            ps,
            noMargin,
        }.attachTo(this);
    }
};

class CMakeProjectSettingsPanelFactory final : public ProjectPanelFactory
{
public:
    CMakeProjectSettingsPanelFactory()
    {
        setPriority(120);
        setDisplayName("CMake");
        setSupportsFunction([](Project *project) {
            return qobject_cast<CMakeProject *>(project) != nullptr;
        });
        setCreateWidgetFunction([](Project *project) {
            return new CMakeProjectSettingsWidget(project);
        });
    }
};

const CMakeProjectSettingsPanelFactory projectSettingsPane;

} // CMakeProjectManager::Internal
