// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectexplorersettings.h"

#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "projectpanelfactory.h"
#include "runcontrol.h"
#include "target.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <utils/environmentdialog.h>
#include <utils/layoutbuilder.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {

ProjectExplorerSettings::ProjectExplorerSettings(bool global)
{
    setSettingsGroups("ProjectExplorer", "Settings");
    setAutoApply(false);

    closeSourceFilesWithProject.setSettingsKey("CloseFilesWithProject");
    closeSourceFilesWithProject.setDefaultValue(true);
    closeSourceFilesWithProject.setLabel(Tr::tr("Close source files along with project"));
    closeSourceFilesWithProject.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    saveBeforeBuild.setSettingsKey("SaveBeforeBuild");
    saveBeforeBuild.setLabel(Tr::tr("Save all files before build"));
    saveBeforeBuild.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    deployBeforeRun.setSettingsKey("DeployBeforeRun");
    deployBeforeRun.setDefaultValue(true);
    deployBeforeRun.setLabelText(Tr::tr("Always deploy project before running it"));
    deployBeforeRun.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    addLibraryPathsToRunEnv.setSettingsKey("AddLibraryPathsToRunEnv");
    addLibraryPathsToRunEnv.setDefaultValue(true);
    addLibraryPathsToRunEnv.setLabel(Tr::tr("Add linker library search paths to run environment"));
    addLibraryPathsToRunEnv.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    promptToStopRunControl.setSettingsKey("PromptToStopRunControl");
    promptToStopRunControl.setLabel(Tr::tr("Always ask before stopping applications"));
    promptToStopRunControl.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    automaticallyCreateRunConfigurations.setSettingsKey("AutomaticallyCreateRunConfigurations");
    automaticallyCreateRunConfigurations.setDefaultValue(true);
    automaticallyCreateRunConfigurations.setLabel(Tr::tr("Create suitable run configurations automatically"));
    automaticallyCreateRunConfigurations.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    syncRunConfigurations.setSettingsKey("SyncRunConfigurations");
    syncRunConfigurations.setLabelText(Tr::tr("Keep run configurations in sync:"));
    syncRunConfigurations.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    syncRunConfigurations.setDefaultValue(SyncRunConfigs::Off);
    syncRunConfigurations.setToolTip(
        Tr::tr(
            "Whether adding, removing or editing a run configuration in one build configuration "
            "should update other build configurations accordingly."));
    syncRunConfigurations.addOption(
        {Tr::tr("Do Not Sync"),
         Tr::tr("All build configurations have their own set of run configurations."),
         int(SyncRunConfigs::Off)});
    syncRunConfigurations.addOption(
        {Tr::tr("Sync Within One Kit"),
         Tr::tr("Build configurations in the same kit keep their run configurations in sync."),
         int(SyncRunConfigs::SameKit)});
    syncRunConfigurations.addOption(
        {Tr::tr("Sync Across All Kits"),
         Tr::tr(
             "All build configurations in a project keep their run configurations in sync, "
             "even across kits."),
         int(SyncRunConfigs::All)});

    clearIssuesOnRebuild.setSettingsKey("ClearIssuesOnRebuild");
    clearIssuesOnRebuild.setDefaultValue(true);
    clearIssuesOnRebuild.setLabel(Tr::tr("Clear issues list on new build"));
    clearIssuesOnRebuild.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    abortBuildAllOnError.setSettingsKey("AbortBuildAllOnError");
    abortBuildAllOnError.setDefaultValue(true);
    abortBuildAllOnError.setLabel(Tr::tr("Abort on error when building all projects"));
    abortBuildAllOnError.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    lowBuildPriority.setSettingsKey("LowBuildPriority");
    lowBuildPriority.setDefaultValue(false);
    lowBuildPriority.setLabel(Tr::tr("Start build processes with low priority"));
    lowBuildPriority.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    warnAgainstNonAsciiBuildDir.setSettingsKey("WarnAgainstNonAsciiBuildDir");
    warnAgainstNonAsciiBuildDir.setDefaultValue(true);
    warnAgainstNonAsciiBuildDir.setLabel(
        Tr::tr("Warn against build directories with spaces or non-ASCII characters"));
    warnAgainstNonAsciiBuildDir.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    warnAgainstNonAsciiBuildDir.setToolTip(
        Tr::tr("Some legacy build tools do not deal well with paths that contain \"special\" "
               "characters such as spaces, potentially resulting in spurious build errors.<p>"
               "Uncheck this option if you do not work with such tools."));

    kitFilter.setSettingsKey("ShowAllKits");
    kitFilter.setDefaultValue(KitFilter::ShowAll);
    kitFilter.addOption({Tr::tr("Show All Kits"), {}, int(KitFilter::ShowAll)});
    kitFilter.addOption({Tr::tr("Show Only Suitable Kits"), {}, int(KitFilter::ShowOnlyMatching)});
    kitFilter.addOption({Tr::tr("Show Only Active Kits"), {}, int(KitFilter::ShowOnlyActive)});
    kitFilter.setLabelText(Tr::tr("Kits listed in \"Projects\" mode:"));
    kitFilter.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);

    buildBeforeDeploy.setSettingsKey("BuildBeforeDeploy");
    buildBeforeDeploy.setDefaultValue(BuildBeforeRunMode::WholeProject);
    buildBeforeDeploy.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    buildBeforeDeploy.setLabelText(Tr::tr("Build before deploying:"));
    buildBeforeDeploy.addOption(Tr::tr("Do Not Build Anything"));
    buildBeforeDeploy.addOption(Tr::tr("Build the Whole Project"));
    buildBeforeDeploy.addOption(Tr::tr("Build Only the Application to Be Run"));

    stopBeforeBuild.setSettingsKey("StopBeforeBuild");
    stopBeforeBuild.setDefaultValue(HostOsInfo::isWindowsHost()
                                      ? StopBeforeBuild::SameProject
                                      : StopBeforeBuild::None);
    stopBeforeBuild.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    stopBeforeBuild.setLabelText(Tr::tr("Stop applications before building:"));
    stopBeforeBuild.addOption({Tr::tr("None", "Stop applications before building: None"), {},
                              int(StopBeforeBuild::None)});
    stopBeforeBuild.addOption({Tr::tr("Same Project"), {}, int(StopBeforeBuild::SameProject)});
    stopBeforeBuild.addOption({Tr::tr("All", "Stop all projects"), {}, int(StopBeforeBuild::All)});
    stopBeforeBuild.addOption({Tr::tr("Same Build Directory"), {}, int(StopBeforeBuild::SameBuildDir)});
    stopBeforeBuild.addOption({Tr::tr("Same Application"), {}, int(StopBeforeBuild::SameApp)});

    terminalMode.setSettingsKey("TerminalMode");
    terminalMode.setDefaultValue(TerminalMode::Off);
    terminalMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    terminalMode.setLabelText(Tr::tr("Default for \"Run in terminal\":"));
    terminalMode.addOption(Tr::tr("Enabled"));
    terminalMode.addOption(Tr::tr("Disabled"));
    terminalMode.addOption(Tr::tr("Deduced from Project"));

    reaperTimeoutInSeconds.setSettingsKey("ReaperTimeout");
    reaperTimeoutInSeconds.setDefaultValue(1);
    reaperTimeoutInSeconds.setRange(1, 100'000);
    //: Suffix for "seconds"
    reaperTimeoutInSeconds.setSuffix(Tr::tr("s"));
    reaperTimeoutInSeconds.setLabelText(Tr::tr("Time to wait before force-stopping applications:"));
    reaperTimeoutInSeconds.setToolTip(
        Tr::tr("The amount of seconds to wait between a \"soft kill\" and a \"hard kill\" of a "
               "running application."));

    useJom.setSettingsKey("UseJom");
    useJom.setDefaultValue(true);
    useJom.setLabel(Tr::tr("Use jom instead of nmake"));
    useJom.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    appEnvChanges.setSettingsKey("AppEnvChanges");
    // appEnvChanges = EnvironmentItem::fromStringList(
    //     s->value("ProjectExplorer/Settings/AppEnvChanes").toStringList());

    environmentId.setSettingsKey("EnvironmentId");

    if (global)
        readSettings();

    // Fix up pre-Qt 4.12
    const QVariant value = ICore::settings()->value("ProjectExplorer/Settings/BuildBeforeDeploy");
    const QString buildBeforeDeployString = value.toString();
    if (buildBeforeDeployString == "true") { // backward compatibility with QtC < 4.12
        buildBeforeDeploy.setValue(BuildBeforeRunMode::WholeProject);
    } else if (buildBeforeDeployString == "false") {
        buildBeforeDeploy.setValue(BuildBeforeRunMode::Off);
    } else if (value.isValid()) {
        buildBeforeDeploy.setValue(static_cast<BuildBeforeRunMode>(value.toInt()));
    }

    if (global && environmentId().isNull()) {
        environmentId.setValue(QUuid::createUuid().toByteArray());
        environmentId.setSettingsKey("ProjectExplorer/Settings/EnvironmentId");
        environmentId.writeSettings();
    }

    if (int(stopBeforeBuild()) < 0 || int(stopBeforeBuild()) > int(StopBeforeBuild::SameApp)) {
        stopBeforeBuild.setValue(stopBeforeBuild.defaultValue());
        writeSettings();
    }
}

ProjectExplorerSettings &ProjectExplorerSettings::get(QObject *context)
{
    if (const Project * const project = projectForContext(context))
        return project->projectExplorerSettings().current();
    return globalProjectExplorerSettings();
}

ProjectExplorerSettings &ProjectExplorerSettings::get(const QObject *context)
{
    return get(const_cast<QObject *>(context));
}

Project *ProjectExplorerSettings::projectForContext(QObject *context)
{
    if (const auto project = qobject_cast<Project *>(context))
        return project;
    if (const auto target = qobject_cast<Target *>(context))
        return target->project();
    if (const auto runControl = qobject_cast<RunControl *>(context))
        return runControl->project();
    if (const auto projectConf = qobject_cast<ProjectConfiguration *>(context))
        return projectConf->project();
    if (const auto aspect = qobject_cast<BaseAspect *>(context))
        return projectForContext(aspect->container());
    return nullptr;
}

namespace Internal {

enum { UseCurrentDirectory, UseProjectDirectory };

void setPromptToStopSettings(bool promptToStop)
{
    globalProjectExplorerSettings().promptToStopRunControl.setValue(promptToStop);
    globalProjectExplorerSettings().writeSettings();
    emit globalProjectExplorerSettings().changed();
}

void setSaveBeforeBuildSettings(bool saveBeforeBuild)
{
    globalProjectExplorerSettings().saveBeforeBuild.setValue(saveBeforeBuild);
    globalProjectExplorerSettings().writeSettings();
}

class ProjectExplorerSettingsWidget : public IOptionsPageWidget
{
public:
    ProjectExplorerSettingsWidget();

    FilePath projectsDirectory() const;
    void setProjectsDirectory(const FilePath &pd);

    bool useProjectsDirectory();
    void setUseProjectsDirectory(bool v);

    void apply() final
    {
        if (globalProjectExplorerSettings().isDirty()) {
            globalProjectExplorerSettings().apply();
            globalProjectExplorerSettings().writeSettings();
        }

        DocumentManager::setProjectsDirectory(projectsDirectory());
        DocumentManager::setUseProjectsDirectory(useProjectsDirectory());
    }

    void cancel() final
    {
        globalProjectExplorerSettings().cancel();
    }

private:
    void slotDirectoryButtonGroupChanged();
    void updateAppEnvChangesLabel();

    QRadioButton *m_currentDirectoryRadioButton;
    QRadioButton *m_directoryRadioButton;
    PathChooser *m_projectsDirectoryPathChooser;

    ElidingLabel *m_appEnvLabel;

    QButtonGroup *m_directoryButtonGroup;
};

ProjectExplorerSettingsWidget::ProjectExplorerSettingsWidget()
{
    ProjectExplorerSettings &s = globalProjectExplorerSettings();

    m_currentDirectoryRadioButton = new QRadioButton(Tr::tr("Current directory"));
    m_directoryRadioButton = new QRadioButton(Tr::tr("Directory"));
    m_projectsDirectoryPathChooser = new PathChooser;

    const QString appEnvToolTip = Tr::tr("Environment changes to apply to run configurations, "
                                         "but not build configurations.");
    const auto appEnvDescriptionLabel = new QLabel(Tr::tr("Application environment:"));
    appEnvDescriptionLabel->setToolTip(appEnvToolTip);
    m_appEnvLabel = new Utils::ElidingLabel;
    m_appEnvLabel->setElideMode(Qt::ElideRight);
    m_appEnvLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const auto appEnvButton = new QPushButton(Tr::tr("Change..."));
    appEnvButton->setSizePolicy(QSizePolicy::Fixed, appEnvButton->sizePolicy().verticalPolicy());
    appEnvButton->setToolTip(appEnvToolTip);
    connect(appEnvButton, &QPushButton::clicked, this, [appEnvButton, &s, this] {
        const std::optional<EnvironmentItems> changes =
                runEnvironmentItemsDialog(appEnvButton, s.appEnvChanges.volatileValue());
        if (!changes)
            return;
        s.appEnvChanges.setVolatileValue(*changes);
        updateAppEnvChangesLabel();
    });

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Projects Directory")),
            Column {
                m_currentDirectoryRadioButton,
                Row { m_directoryRadioButton, m_projectsDirectoryPathChooser },
            },
        },
        Group {
            title(Tr::tr("Closing Projects")),
            Column {
                s.closeSourceFilesWithProject
            },
        },
        Group {
            title(Tr::tr("Build and Run")),
            Column {
                s.saveBeforeBuild,
                s.deployBeforeRun,
                s.addLibraryPathsToRunEnv,
                s.promptToStopRunControl,
                s.automaticallyCreateRunConfigurations,
                s.clearIssuesOnRebuild,
                s.abortBuildAllOnError,
                s.lowBuildPriority,
                s.warnAgainstNonAsciiBuildDir,
                Form {
                    s.kitFilter, br,
                    appEnvDescriptionLabel, Row{m_appEnvLabel, appEnvButton, st}, br,
                    s.buildBeforeDeploy, br,
                    s.stopBeforeBuild, br,
                    s.terminalMode, br,
                    s.syncRunConfigurations, br,
                    s.reaperTimeoutInSeconds, st, br,
                },
                If (HostOsInfo::isWindowsHost()) >> Then {
                    Label {
                        text("<i>jom</i> is a drop-in replacement for <i>nmake</i> which "
                             "distributes the compilation process to multiple CPU cores. "
                             "The latest binary is available at "
                             "<a href=\"http://download.qt.io/official_releases/jom/\">"
                             "http://download.qt.io/official_releases/jom/</a>. "
                             "Disable it if you experience problems with your builds."),
                        wordWrap(true)
                    },
                    s.useJom,
                }
            },
        },
        st,
    }.attachTo(this);

    m_directoryButtonGroup = new QButtonGroup(this);
    m_directoryButtonGroup->setExclusive(true);
    m_directoryButtonGroup->addButton(m_currentDirectoryRadioButton, UseCurrentDirectory);
    m_directoryButtonGroup->addButton(m_directoryRadioButton, UseProjectDirectory);

    connect(m_directoryButtonGroup, &QButtonGroup::buttonClicked,
            this, &ProjectExplorerSettingsWidget::slotDirectoryButtonGroupChanged);

    setProjectsDirectory(DocumentManager::projectsDirectory());
    setUseProjectsDirectory(DocumentManager::useProjectsDirectory());
    updateAppEnvChangesLabel();
}

FilePath ProjectExplorerSettingsWidget::projectsDirectory() const
{
    return m_projectsDirectoryPathChooser->filePath();
}

void ProjectExplorerSettingsWidget::setProjectsDirectory(const FilePath &pd)
{
    m_projectsDirectoryPathChooser->setFilePath(pd);
}

bool ProjectExplorerSettingsWidget::useProjectsDirectory()
{
    return m_directoryButtonGroup->checkedId() == UseProjectDirectory;
}

void ProjectExplorerSettingsWidget::setUseProjectsDirectory(bool b)
{
    if (useProjectsDirectory() != b) {
        (b ? m_directoryRadioButton : m_currentDirectoryRadioButton)->setChecked(true);
        slotDirectoryButtonGroupChanged();
    }
}

void ProjectExplorerSettingsWidget::slotDirectoryButtonGroupChanged()
{
    bool enable = useProjectsDirectory();
    m_projectsDirectoryPathChooser->setEnabled(enable);
}

void ProjectExplorerSettingsWidget::updateAppEnvChangesLabel()
{
    const EnvironmentItems changes = globalProjectExplorerSettings().appEnvChanges.volatileValue();
    m_appEnvLabel->setText(EnvironmentItem::toShortSummary(changes));
}

// ProjectExplorerSettingsPage

class ProjectExplorerSettingsPage final : public IOptionsPage
{
public:
    ProjectExplorerSettingsPage()
    {
        setId(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("General"));
        setCategory(ProjectExplorer::Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new ProjectExplorerSettingsWidget; });
    }
};

void setupProjectExplorerSettings()
{
    static ProjectExplorerSettingsPage theProjectExplorerSettingsPage;
}

class ProjectExplorerSettingsProjectPanelFactory final : public ProjectPanelFactory
{
public:
    ProjectExplorerSettingsProjectPanelFactory()
    {
        setPriority(10);
        setId("ProjectExplorer.BuildAndRunSettings");
        setDisplayName(Tr::tr("Building and Running"));
        setCreateWidgetFunction([](Project *project) {
            return project->projectExplorerSettings().createConfigWidget();
        });
    }
};

void setupProjectExplorerSettingsProjectPanel()
{
    static ProjectExplorerSettingsProjectPanelFactory theFactory;
}

} // Internal

ProjectExplorerSettings &globalProjectExplorerSettings()
{
    static ProjectExplorerSettings theProjectExplorerSettings(true);
    return theProjectExplorerSettings;
}

PerProjectProjectExplorerSettings::PerProjectProjectExplorerSettings(Project *project)
    : m_project(project)
{
    QTC_ASSERT(project, return);

    const auto settings = new ProjectExplorerSettings(false);
    setProjectSettings(settings);
    setGlobalSettings(&globalProjectExplorerSettings(), Constants::BUILD_AND_RUN_SETTINGS_PAGE_ID);
    setId("PESettingsAspect");
    settings->setSettingsKey("PESettings");
    settings->setLayouter([settings] {
        using namespace Layouting;
        return Column {
            settings->addLibraryPathsToRunEnv,
            settings->automaticallyCreateRunConfigurations,
            settings->lowBuildPriority,
            settings->warnAgainstNonAsciiBuildDir,

            Form {
                settings->terminalMode, br,
                settings->syncRunConfigurations, br,
                settings->reaperTimeoutInSeconds, st, br,
            },
            st,
        };
    });
    setDisplayName(Tr::tr("Building and Running"));
    setUsingGlobalSettings(true);
    resetProjectToGlobalSettings();
    setConfigWidgetCreator([this] { return createGlobalOrProjectAspectWidget(this); });

    const auto save = [this, project] {
        Store map;
        toMap(map);
        project->setNamedSettings(id().toKey(), mapFromStore(map));
    };
    connect(settings, &BaseAspect::changed, this, save);
    connect(this, &GlobalOrProjectAspect::currentSettingsChanged, this, save);
}

ProjectExplorerSettings &PerProjectProjectExplorerSettings::custom()
{
    return *static_cast<ProjectExplorerSettings *>(projectSettings());
}

ProjectExplorerSettings &PerProjectProjectExplorerSettings::current()
{
    return *static_cast<ProjectExplorerSettings *>(currentSettings());
}

void PerProjectProjectExplorerSettings::restore()
{
    fromMap(storeFromVariant(m_project->namedSettings(id().toKey())));
}

} // ProjectExplorer

