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
namespace Internal {

namespace Constants {
const char REAPER_TIMEOUT_SETTINGS_KEY[] = "ProjectExplorer/Settings/ReaperTimeout";
const char BUILD_BEFORE_DEPLOY_SETTINGS_KEY[] = "ProjectExplorer/Settings/BuildBeforeDeploy";
const char DEPLOY_BEFORE_RUN_SETTINGS_KEY[] = "ProjectExplorer/Settings/DeployBeforeRun";
const char SAVE_BEFORE_BUILD_SETTINGS_KEY[] = "ProjectExplorer/Settings/SaveBeforeBuild";
const char USE_JOM_SETTINGS_KEY[] = "ProjectExplorer/Settings/UseJom";
const char ADD_LIBRARY_PATHS_TO_RUN_ENV_SETTINGS_KEY[] =
        "ProjectExplorer/Settings/AddLibraryPathsToRunEnv";
const char PROMPT_TO_STOP_RUN_CONTROL_SETTINGS_KEY[] =
        "ProjectExplorer/Settings/PromptToStopRunControl";
const char AUTO_CREATE_RUN_CONFIGS_SETTINGS_KEY[] =
        "ProjectExplorer/Settings/AutomaticallyCreateRunConfigurations";
const char ENVIRONMENT_ID_SETTINGS_KEY[] = "ProjectExplorer/Settings/EnvironmentId";
const char STOP_BEFORE_BUILD_SETTINGS_KEY[] = "ProjectExplorer/Settings/StopBeforeBuild";
const char TERMINAL_MODE_SETTINGS_KEY[] = "ProjectExplorer/Settings/TerminalMode";
const char CLOSE_FILES_WITH_PROJECT_SETTINGS_KEY[]
    = "ProjectExplorer/Settings/CloseFilesWithProject";
const char CLEAR_ISSUES_ON_REBUILD_SETTINGS_KEY[] = "ProjectExplorer/Settings/ClearIssuesOnRebuild";
const char ABORT_BUILD_ALL_ON_ERROR_SETTINGS_KEY[]
    = "ProjectExplorer/Settings/AbortBuildAllOnError";
const char LOW_BUILD_PRIORITY_SETTINGS_KEY[] = "ProjectExplorer/Settings/LowBuildPriority";
const char APP_ENV_CHANGES_SETTINGS_KEY[] = "ProjectExplorer/Settings/AppEnvChanges";
const char WARN_AGAINST_NON_ASCII_BUILD_DIR_SETTINGS_KEY[]
    = "ProjectExplorer/Settings/WarnAgainstNonAsciiBuildDir";

} // Constants

enum { UseCurrentDirectory, UseProjectDirectory };

void saveProjectExplorerSettings();

static bool operator==(const ProjectExplorerSettings &p1, const ProjectExplorerSettings &p2)
{
    return p1.buildBeforeDeploy == p2.buildBeforeDeploy
            && p1.reaperTimeoutInSeconds == p2.reaperTimeoutInSeconds
            && p1.deployBeforeRun == p2.deployBeforeRun
            && p1.saveBeforeBuild == p2.saveBeforeBuild
            && p1.useJom == p2.useJom
            && p1.prompToStopRunControl == p2.prompToStopRunControl
            && p1.automaticallyCreateRunConfigurations == p2.automaticallyCreateRunConfigurations
            && p1.addLibraryPathsToRunEnv == p2.addLibraryPathsToRunEnv
            && p1.environmentId == p2.environmentId
            && p1.stopBeforeBuild == p2.stopBeforeBuild
            && p1.terminalMode == p2.terminalMode
            && p1.closeSourceFilesWithProject == p2.closeSourceFilesWithProject
            && p1.clearIssuesOnRebuild == p2.clearIssuesOnRebuild
            && p1.abortBuildAllOnError == p2.abortBuildAllOnError
            && p1.appEnvChanges == p2.appEnvChanges
            && p1.lowBuildPriority == p2.lowBuildPriority
            && p1.warnAgainstNonAsciiBuildDir == p2.warnAgainstNonAsciiBuildDir
            && p1.showAllKits == p2.showAllKits;
}

ProjectExplorerSettings &mutableProjectExplorerSettings()
{
    static ProjectExplorerSettings theProjectExplorerSettings;
    return theProjectExplorerSettings;
}

void setPromptToStopSettings(bool promptToStop)
{
    mutableProjectExplorerSettings().prompToStopRunControl = promptToStop;
    saveProjectExplorerSettings();
    emit ProjectExplorerPlugin::instance()->settingsChanged();
}

void setSaveBeforeBuildSettings(bool saveBeforeBuild)
{
    mutableProjectExplorerSettings().saveBeforeBuild = saveBeforeBuild;
    saveProjectExplorerSettings();
}

static void loadProjectExplorerSettings()
{
    QtcSettings *s = ICore::settings();

    const QVariant buildBeforeDeploy = s->value(Constants::BUILD_BEFORE_DEPLOY_SETTINGS_KEY);
    const QString buildBeforeDeployString = buildBeforeDeploy.toString();
    ProjectExplorerSettings &settings = mutableProjectExplorerSettings();
    if (buildBeforeDeployString == "true") { // backward compatibility with QtC < 4.12
        settings.buildBeforeDeploy = BuildBeforeRunMode::WholeProject;
    } else if (buildBeforeDeployString == "false") {
        settings.buildBeforeDeploy = BuildBeforeRunMode::Off;
    } else if (buildBeforeDeploy.isValid()) {
        settings.buildBeforeDeploy
                = static_cast<BuildBeforeRunMode>(buildBeforeDeploy.toInt());
    }

    static const ProjectExplorerSettings defaultSettings;

    settings.reaperTimeoutInSeconds
        = s->value(Constants::REAPER_TIMEOUT_SETTINGS_KEY, defaultSettings.reaperTimeoutInSeconds)
              .toInt();
    settings.deployBeforeRun
        = s->value(Constants::DEPLOY_BEFORE_RUN_SETTINGS_KEY, defaultSettings.deployBeforeRun)
              .toBool();
    settings.saveBeforeBuild
        = s->value(Constants::SAVE_BEFORE_BUILD_SETTINGS_KEY, defaultSettings.saveBeforeBuild)
              .toBool();
    settings.useJom
        = s->value(Constants::USE_JOM_SETTINGS_KEY, defaultSettings.useJom).toBool();
    settings.addLibraryPathsToRunEnv
        = s->value(Constants::ADD_LIBRARY_PATHS_TO_RUN_ENV_SETTINGS_KEY,
                   defaultSettings.addLibraryPathsToRunEnv)
              .toBool();
    settings.prompToStopRunControl
        = s->value(Constants::PROMPT_TO_STOP_RUN_CONTROL_SETTINGS_KEY,
                   defaultSettings.prompToStopRunControl)
              .toBool();
    settings.automaticallyCreateRunConfigurations
        = s->value(Constants::AUTO_CREATE_RUN_CONFIGS_SETTINGS_KEY,
                   defaultSettings.automaticallyCreateRunConfigurations)
              .toBool();
    settings.environmentId =
            QUuid(s->value(Constants::ENVIRONMENT_ID_SETTINGS_KEY).toByteArray());
    if (settings.environmentId.isNull()) {
        settings.environmentId = QUuid::createUuid();
        s->setValue(Constants::ENVIRONMENT_ID_SETTINGS_KEY, settings.environmentId.toByteArray());
    }
    int tmp = s->value(Constants::STOP_BEFORE_BUILD_SETTINGS_KEY,
                       int(defaultSettings.stopBeforeBuild))
                  .toInt();
    if (tmp < 0 || tmp > int(StopBeforeBuild::SameApp))
        tmp = int(defaultSettings.stopBeforeBuild);
    settings.stopBeforeBuild = StopBeforeBuild(tmp);
    settings.terminalMode = static_cast<TerminalMode>(
        s->value(Constants::TERMINAL_MODE_SETTINGS_KEY, int(defaultSettings.terminalMode)).toInt());
    settings.closeSourceFilesWithProject
        = s->value(Constants::CLOSE_FILES_WITH_PROJECT_SETTINGS_KEY,
                   defaultSettings.closeSourceFilesWithProject)
              .toBool();
    settings.clearIssuesOnRebuild
        = s->value(Constants::CLEAR_ISSUES_ON_REBUILD_SETTINGS_KEY,
                   defaultSettings.clearIssuesOnRebuild)
              .toBool();
    settings.abortBuildAllOnError
        = s->value(Constants::ABORT_BUILD_ALL_ON_ERROR_SETTINGS_KEY,
                   defaultSettings.abortBuildAllOnError)
              .toBool();
    settings.lowBuildPriority
        = s->value(Constants::LOW_BUILD_PRIORITY_SETTINGS_KEY, defaultSettings.lowBuildPriority)
              .toBool();
    settings.warnAgainstNonAsciiBuildDir
        = s->value(Constants::WARN_AGAINST_NON_ASCII_BUILD_DIR_SETTINGS_KEY,
                   defaultSettings.warnAgainstNonAsciiBuildDir)
              .toBool();
    settings.appEnvChanges = EnvironmentItem::fromStringList(
        s->value(Constants::APP_ENV_CHANGES_SETTINGS_KEY).toStringList());
    settings.showAllKits
        = s->value(ProjectExplorer::Constants::SHOW_ALL_KITS_SETTINGS_KEY, defaultSettings.showAllKits)
              .toBool();
}

void saveProjectExplorerSettings()
{
    QtcSettings *s = ICore::settings();
    static const ProjectExplorerSettings defaultSettings;

    const ProjectExplorerSettings &settings = projectExplorerSettings();
    s->setValueWithDefault(
        Constants::REAPER_TIMEOUT_SETTINGS_KEY,
        settings.reaperTimeoutInSeconds,
        defaultSettings.reaperTimeoutInSeconds);
    s->setValueWithDefault(Constants::BUILD_BEFORE_DEPLOY_SETTINGS_KEY,
                           int(settings.buildBeforeDeploy),
                           int(defaultSettings.buildBeforeDeploy));
    s->setValueWithDefault(Constants::DEPLOY_BEFORE_RUN_SETTINGS_KEY,
                           settings.deployBeforeRun,
                           defaultSettings.deployBeforeRun);
    s->setValueWithDefault(Constants::SAVE_BEFORE_BUILD_SETTINGS_KEY,
                           settings.saveBeforeBuild,
                           defaultSettings.saveBeforeBuild);
    s->setValueWithDefault(Constants::USE_JOM_SETTINGS_KEY,
                           settings.useJom,
                           defaultSettings.useJom);
    s->setValueWithDefault(Constants::ADD_LIBRARY_PATHS_TO_RUN_ENV_SETTINGS_KEY,
                           settings.addLibraryPathsToRunEnv,
                           defaultSettings.addLibraryPathsToRunEnv);
    s->setValueWithDefault(Constants::PROMPT_TO_STOP_RUN_CONTROL_SETTINGS_KEY,
                           settings.prompToStopRunControl,
                           defaultSettings.prompToStopRunControl);
    s->setValueWithDefault(Constants::TERMINAL_MODE_SETTINGS_KEY,
                           int(settings.terminalMode),
                           int(defaultSettings.terminalMode));
    s->setValueWithDefault(Constants::CLOSE_FILES_WITH_PROJECT_SETTINGS_KEY,
                           settings.closeSourceFilesWithProject,
                           defaultSettings.closeSourceFilesWithProject);
    s->setValueWithDefault(Constants::CLEAR_ISSUES_ON_REBUILD_SETTINGS_KEY,
                           settings.clearIssuesOnRebuild,
                           defaultSettings.clearIssuesOnRebuild);
    s->setValueWithDefault(Constants::ABORT_BUILD_ALL_ON_ERROR_SETTINGS_KEY,
                           settings.abortBuildAllOnError,
                           defaultSettings.abortBuildAllOnError);
    s->setValueWithDefault(Constants::LOW_BUILD_PRIORITY_SETTINGS_KEY,
                           settings.lowBuildPriority,
                           defaultSettings.lowBuildPriority);
    s->setValueWithDefault(Constants::WARN_AGAINST_NON_ASCII_BUILD_DIR_SETTINGS_KEY,
                           settings.warnAgainstNonAsciiBuildDir,
                           defaultSettings.warnAgainstNonAsciiBuildDir);
    s->setValueWithDefault(Constants::AUTO_CREATE_RUN_CONFIGS_SETTINGS_KEY,
                           settings.automaticallyCreateRunConfigurations,
                           defaultSettings.automaticallyCreateRunConfigurations);
    s->setValueWithDefault(Constants::ENVIRONMENT_ID_SETTINGS_KEY,
                           settings.environmentId.toByteArray());
    s->setValueWithDefault(Constants::STOP_BEFORE_BUILD_SETTINGS_KEY,
                           int(settings.stopBeforeBuild),
                           int(defaultSettings.stopBeforeBuild));
    s->setValueWithDefault(Constants::APP_ENV_CHANGES_SETTINGS_KEY,
                           EnvironmentItem::toStringList(settings.appEnvChanges));
    s->setValueWithDefault(
        ProjectExplorer::Constants::SHOW_ALL_KITS_SETTINGS_KEY,
        settings.showAllKits,
        defaultSettings.showAllKits);
}

class ProjectExplorerSettingsWidget : public IOptionsPageWidget
{
public:
    ProjectExplorerSettingsWidget();

    ProjectExplorerSettings settings() const;
    void setSettings(const ProjectExplorerSettings  &s);

    FilePath projectsDirectory() const;
    void setProjectsDirectory(const FilePath &pd);

    bool useProjectsDirectory();
    void setUseProjectsDirectory(bool v);

    void apply() final
    {
        ProjectExplorerSettings newSettings = settings();
        QTC_CHECK(projectExplorerSettings().environmentId == newSettings.environmentId);

        if (!(projectExplorerSettings() == newSettings)) {
            mutableProjectExplorerSettings() = newSettings;
            saveProjectExplorerSettings();
            emit ProjectExplorerPlugin::instance()->settingsChanged();
        }

        DocumentManager::setProjectsDirectory(projectsDirectory());
        DocumentManager::setUseProjectsDirectory(useProjectsDirectory());
    }

private:
    void slotDirectoryButtonGroupChanged();
    void updateAppEnvChangesLabel();

    Utils::EnvironmentItems m_appEnvChanges;
    QRadioButton *m_currentDirectoryRadioButton;
    QRadioButton *m_directoryRadioButton;
    PathChooser *m_projectsDirectoryPathChooser;
    QCheckBox *m_closeSourceFilesCheckBox;
    QCheckBox *m_saveAllFilesCheckBox;
    QCheckBox *m_deployProjectBeforeRunCheckBox;
    QCheckBox *m_addLibraryPathsToRunEnvCheckBox;
    QCheckBox *m_promptToStopRunControlCheckBox;
    QCheckBox *m_automaticallyCreateRunConfiguration;
    QCheckBox *m_clearIssuesCheckBox;
    QCheckBox *m_abortBuildAllOnErrorCheckBox;
    QCheckBox *m_lowBuildPriorityCheckBox;
    QCheckBox *m_warnAgainstNonAsciiBuildDirCheckBox;
    QComboBox *m_buildBeforeDeployComboBox;
    QComboBox *m_stopBeforeBuildComboBox;
    QComboBox *m_terminalModeComboBox;
    QCheckBox *m_jomCheckbox;
    QCheckBox *m_showAllKitsCheckBox;
    QSpinBox *m_reaperTimeoutSpinBox;
    Utils::ElidingLabel *m_appEnvLabel;

    QButtonGroup *m_directoryButtonGroup;
};

ProjectExplorerSettingsWidget::ProjectExplorerSettingsWidget()
{
    m_reaperTimeoutSpinBox = new QSpinBox;
    m_reaperTimeoutSpinBox->setMinimum(1);
    //: Suffix for "seconds"
    m_reaperTimeoutSpinBox->setSuffix(Tr::tr("s"));
    m_reaperTimeoutSpinBox->setToolTip(
        Tr::tr("The amount of seconds to wait between a \"soft kill\" and a \"hard kill\" of a "
               "running application."));

    m_currentDirectoryRadioButton = new QRadioButton(Tr::tr("Current directory"));
    m_directoryRadioButton = new QRadioButton(Tr::tr("Directory"));
    m_projectsDirectoryPathChooser = new PathChooser;
    m_closeSourceFilesCheckBox = new QCheckBox(Tr::tr("Close source files along with project"));
    m_saveAllFilesCheckBox = new QCheckBox(Tr::tr("Save all files before build"));
    m_deployProjectBeforeRunCheckBox = new QCheckBox(Tr::tr("Always deploy project before running it"));
    m_addLibraryPathsToRunEnvCheckBox =
            new QCheckBox(Tr::tr("Add linker library search paths to run environment"));
    m_promptToStopRunControlCheckBox = new QCheckBox(Tr::tr("Always ask before stopping applications"));
    m_automaticallyCreateRunConfiguration =
            new QCheckBox(Tr::tr("Create suitable run configurations automatically"));
    m_clearIssuesCheckBox = new QCheckBox(Tr::tr("Clear issues list on new build"));
    m_abortBuildAllOnErrorCheckBox = new QCheckBox(Tr::tr("Abort on error when building all projects"));
    m_lowBuildPriorityCheckBox = new QCheckBox(Tr::tr("Start build processes with low priority"));
    m_warnAgainstNonAsciiBuildDirCheckBox = new QCheckBox(
        Tr::tr("Warn against build directories with spaces or non-ASCII characters"));
    m_warnAgainstNonAsciiBuildDirCheckBox->setToolTip(
        Tr::tr("Some legacy build tools do not deal well with paths that contain \"special\" "
               "characters such as spaces, potentially resulting in spurious build errors.<p>"
               "Uncheck this option if you do not work with such tools."));
    m_buildBeforeDeployComboBox = new QComboBox;
    m_buildBeforeDeployComboBox->addItem(Tr::tr("Do Not Build Anything"),
                                         int(BuildBeforeRunMode::Off));
    m_buildBeforeDeployComboBox->addItem(Tr::tr("Build the Whole Project"),
                                         int(BuildBeforeRunMode::WholeProject));
    m_buildBeforeDeployComboBox->addItem(Tr::tr("Build Only the Application to Be Run"),
                                         int(BuildBeforeRunMode::AppOnly));
    const QSizePolicy cbSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_buildBeforeDeployComboBox->setSizePolicy(cbSizePolicy);
    m_stopBeforeBuildComboBox = new QComboBox;
    m_stopBeforeBuildComboBox->addItem(
        Tr::tr("None", "Stop applications before building: None"), int(StopBeforeBuild::None));
    m_stopBeforeBuildComboBox->addItem(Tr::tr("All"), int(StopBeforeBuild::All));
    m_stopBeforeBuildComboBox->addItem(Tr::tr("Same Project"), int(StopBeforeBuild::SameProject));
    m_stopBeforeBuildComboBox->addItem(Tr::tr("Same Build Directory"),
                                       int(StopBeforeBuild::SameBuildDir));
    m_stopBeforeBuildComboBox->addItem(Tr::tr("Same Application"),
                                       int(StopBeforeBuild::SameApp));
    m_stopBeforeBuildComboBox->setSizePolicy(cbSizePolicy);
    m_terminalModeComboBox = new QComboBox;
    m_terminalModeComboBox->addItem(Tr::tr("Enabled"));
    m_terminalModeComboBox->addItem(Tr::tr("Disabled"));
    m_terminalModeComboBox->addItem(Tr::tr("Deduced from Project"));
    m_terminalModeComboBox->setSizePolicy(cbSizePolicy);
    m_jomCheckbox = new QCheckBox(Tr::tr("Use jom instead of nmake"));
    auto jomLabel = new QLabel("<i>jom</i> is a drop-in replacement for <i>nmake</i> which "
                               "distributes the compilation process to multiple CPU cores. "
                               "The latest binary is available at "
                               "<a href=\"http://download.qt.io/official_releases/jom/\">"
                               "http://download.qt.io/official_releases/jom/</a>. "
                               "Disable it if you experience problems with your builds.");
    jomLabel->setWordWrap(true);

    m_showAllKitsCheckBox = new QCheckBox(
        Tr::tr("Show all kits in \"Build & Run\" in \"Projects\" mode"));
    m_showAllKitsCheckBox->setToolTip(
        Tr::tr("Show also inactive kits in \"Build & Run\" in \"Projects\" mode."));

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
    connect(appEnvButton, &QPushButton::clicked, this, [appEnvButton, this] {
        const std::optional<EnvironmentItems> changes =
                runEnvironmentItemsDialog(appEnvButton, m_appEnvChanges);
        if (!changes)
            return;
        m_appEnvChanges = *changes;
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
                m_closeSourceFilesCheckBox,
            },
        },
        Group {
            title(Tr::tr("Build and Run")),
            Column {
                m_saveAllFilesCheckBox,
                m_deployProjectBeforeRunCheckBox,
                m_addLibraryPathsToRunEnvCheckBox,
                m_promptToStopRunControlCheckBox,
                m_automaticallyCreateRunConfiguration,
                m_clearIssuesCheckBox,
                m_abortBuildAllOnErrorCheckBox,
                m_lowBuildPriorityCheckBox,
                m_warnAgainstNonAsciiBuildDirCheckBox,
                m_showAllKitsCheckBox,
                Form {
                    appEnvDescriptionLabel, Row{m_appEnvLabel, appEnvButton, st}, br,
                    Tr::tr("Build before deploying:"), m_buildBeforeDeployComboBox, br,
                    Tr::tr("Stop applications before building:"), m_stopBeforeBuildComboBox, br,
                    Tr::tr("Default for \"Run in terminal\":"), m_terminalModeComboBox, br,
                    Tr::tr("Time to wait before force-stopping applications:"),
                    m_reaperTimeoutSpinBox, st, br,
                },
                m_jomCheckbox,
                jomLabel,
            },
        },
        st,
    }.attachTo(this);

    m_jomCheckbox->setVisible(HostOsInfo::isWindowsHost());
    jomLabel->setVisible(HostOsInfo::isWindowsHost());

    m_directoryButtonGroup = new QButtonGroup(this);
    m_directoryButtonGroup->setExclusive(true);
    m_directoryButtonGroup->addButton(m_currentDirectoryRadioButton, UseCurrentDirectory);
    m_directoryButtonGroup->addButton(m_directoryRadioButton, UseProjectDirectory);

    connect(m_directoryButtonGroup, &QButtonGroup::buttonClicked,
            this, &ProjectExplorerSettingsWidget::slotDirectoryButtonGroupChanged);

    setSettings(projectExplorerSettings());
    setProjectsDirectory(DocumentManager::projectsDirectory());
    setUseProjectsDirectory(DocumentManager::useProjectsDirectory());
    updateAppEnvChangesLabel();
}

ProjectExplorerSettings ProjectExplorerSettingsWidget::settings() const
{
    ProjectExplorerSettings s;
    s.reaperTimeoutInSeconds = m_reaperTimeoutSpinBox->value();
    s.buildBeforeDeploy = static_cast<BuildBeforeRunMode>(
       m_buildBeforeDeployComboBox->currentData().toInt());
    s.deployBeforeRun = m_deployProjectBeforeRunCheckBox->isChecked();
    s.saveBeforeBuild = m_saveAllFilesCheckBox->isChecked();
    s.useJom = m_jomCheckbox->isChecked();
    s.addLibraryPathsToRunEnv = m_addLibraryPathsToRunEnvCheckBox->isChecked();
    s.prompToStopRunControl = m_promptToStopRunControlCheckBox->isChecked();
    s.automaticallyCreateRunConfigurations = m_automaticallyCreateRunConfiguration->isChecked();
    s.stopBeforeBuild = static_cast<StopBeforeBuild>(
       m_stopBeforeBuildComboBox->currentData().toInt());
    s.terminalMode = static_cast<ProjectExplorer::TerminalMode>(m_terminalModeComboBox->currentIndex());
    s.closeSourceFilesWithProject = m_closeSourceFilesCheckBox->isChecked();
    s.clearIssuesOnRebuild = m_clearIssuesCheckBox->isChecked();
    s.abortBuildAllOnError = m_abortBuildAllOnErrorCheckBox->isChecked();
    s.lowBuildPriority = m_lowBuildPriorityCheckBox->isChecked();
    s.warnAgainstNonAsciiBuildDir = m_warnAgainstNonAsciiBuildDirCheckBox->isChecked();
    s.appEnvChanges = m_appEnvChanges;
    s.showAllKits = m_showAllKitsCheckBox->isChecked();
    s.environmentId = projectExplorerSettings().environmentId;
    return s;
}

void ProjectExplorerSettingsWidget::setSettings(const ProjectExplorerSettings  &s)
{
    m_reaperTimeoutSpinBox->setValue(s.reaperTimeoutInSeconds);
    m_appEnvChanges = s.appEnvChanges;
    m_buildBeforeDeployComboBox->setCurrentIndex(
                m_buildBeforeDeployComboBox->findData(int(s.buildBeforeDeploy)));
    m_deployProjectBeforeRunCheckBox->setChecked(s.deployBeforeRun);
    m_saveAllFilesCheckBox->setChecked(s.saveBeforeBuild);
    m_jomCheckbox->setChecked(s.useJom);
    m_addLibraryPathsToRunEnvCheckBox->setChecked(s.addLibraryPathsToRunEnv);
    m_promptToStopRunControlCheckBox->setChecked(s.prompToStopRunControl);
    m_automaticallyCreateRunConfiguration->setChecked(s.automaticallyCreateRunConfigurations);
    m_stopBeforeBuildComboBox->setCurrentIndex(
                m_stopBeforeBuildComboBox->findData(int(s.stopBeforeBuild)));
    m_terminalModeComboBox->setCurrentIndex(static_cast<int>(s.terminalMode));
    m_closeSourceFilesCheckBox->setChecked(s.closeSourceFilesWithProject);
    m_clearIssuesCheckBox->setChecked(s.clearIssuesOnRebuild);
    m_abortBuildAllOnErrorCheckBox->setChecked(s.abortBuildAllOnError);
    m_lowBuildPriorityCheckBox->setChecked(s.lowBuildPriority);
    m_warnAgainstNonAsciiBuildDirCheckBox->setChecked(s.warnAgainstNonAsciiBuildDir);
    m_showAllKitsCheckBox->setChecked(s.showAllKits);
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
    const QString shortSummary = EnvironmentItem::toStringList(m_appEnvChanges).join("; ");
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

    loadProjectExplorerSettings();
}


} // Internal

const ProjectExplorerSettings &projectExplorerSettings()
{
    return Internal::mutableProjectExplorerSettings();
}

ProjectExplorerSettings &mutableProjectExplorerSettings()
{
    return Internal::mutableProjectExplorerSettings();
}

} // ProjectExplorer

