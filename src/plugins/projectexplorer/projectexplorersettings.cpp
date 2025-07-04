// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectexplorersettings.h"

#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"

#include <coreplugin/coreconstants.h>
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

ProjectExplorerSettings::ProjectExplorerSettings()
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

    showAllKits.setSettingsKey("ShowAllKits");
    showAllKits.setDefaultValue(true);
    showAllKits.setLabel(Tr::tr("Show all kits in \"Build & Run\" in \"Projects\" mode"));
    showAllKits.setToolTip(
        Tr::tr("Show also inactive kits in \"Build & Run\" in \"Projects\" mode."));
    showAllKits.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

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

    if (environmentId().isNull()) {
        environmentId.setValue(QUuid::createUuid().toByteArray());
        environmentId.writeSettings();
    }

    if (int(stopBeforeBuild()) < 0 || int(stopBeforeBuild()) > int(StopBeforeBuild::SameApp)) {
        stopBeforeBuild.setValue(stopBeforeBuild.defaultValue());
        stopBeforeBuild.writeSettings();
    }
}

namespace Internal {

enum { UseCurrentDirectory, UseProjectDirectory };

void setPromptToStopSettings(bool promptToStop)
{
    projectExplorerSettings().promptToStopRunControl.setValue(promptToStop);
    projectExplorerSettings().promptToStopRunControl.writeSettings();
    emit ProjectExplorerPlugin::instance()->settingsChanged();
}

void setSaveBeforeBuildSettings(bool saveBeforeBuild)
{
    projectExplorerSettings().saveBeforeBuild.setValue(saveBeforeBuild);
    projectExplorerSettings().saveBeforeBuild.writeSettings();
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
        if (projectExplorerSettings().isDirty()) {
            projectExplorerSettings().apply();
            projectExplorerSettings().writeSettings();
            emit ProjectExplorerPlugin::instance()->settingsChanged();
        }

        DocumentManager::setProjectsDirectory(projectsDirectory());
        DocumentManager::setUseProjectsDirectory(useProjectsDirectory());
    }

    void cancel() final
    {
        projectExplorerSettings().cancel();
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
    ProjectExplorerSettings &s = projectExplorerSettings();

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
                s.showAllKits,

                Form {
                    appEnvDescriptionLabel, Row{m_appEnvLabel, appEnvButton, st}, br,
                    s.buildBeforeDeploy, br,
                    s.stopBeforeBuild, br,
                    s.terminalMode, br,
                    s.reaperTimeoutInSeconds, st, br,
                },
                If { HostOsInfo::isWindowsHost(), {
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
                }}
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
    const EnvironmentItems changes = projectExplorerSettings().appEnvChanges.volatileValue();
    const QString shortSummary = EnvironmentItem::toStringList(changes).join("; ");
    m_appEnvLabel->setText(shortSummary.isEmpty() ? Tr::tr("No changes to apply.")
                                                  : shortSummary);
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

} // Internal

ProjectExplorerSettings &projectExplorerSettings()
{
    static ProjectExplorerSettings theProjectExplorerSettings;
    return theProjectExplorerSettings;
}

} // ProjectExplorer

