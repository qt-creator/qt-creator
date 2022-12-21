// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectexplorersettingspage.h"
#include "projectexplorersettings.h"
#include "projectexplorer.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <utils/layoutbuilder.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QLabel>
#include <QRadioButton>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

enum { UseCurrentDirectory, UseProjectDirectory };

class ProjectExplorerSettingsWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(ProjextExplorer::Internal::ProjectExplorerSettings)

public:
    explicit ProjectExplorerSettingsWidget(QWidget *parent = nullptr);

    ProjectExplorerSettings settings() const;
    void setSettings(const ProjectExplorerSettings  &s);

    FilePath projectsDirectory() const;
    void setProjectsDirectory(const FilePath &pd);

    bool useProjectsDirectory();
    void setUseProjectsDirectory(bool v);

private:
    void slotDirectoryButtonGroupChanged();

    mutable ProjectExplorerSettings m_settings;
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
    QComboBox *m_buildBeforeDeployComboBox;
    QComboBox *m_stopBeforeBuildComboBox;
    QComboBox *m_terminalModeComboBox;
    QCheckBox *m_jomCheckbox;

    QButtonGroup *m_directoryButtonGroup;
};

ProjectExplorerSettingsWidget::ProjectExplorerSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    m_currentDirectoryRadioButton = new QRadioButton(tr("Current directory"));
    m_directoryRadioButton = new QRadioButton(tr("Directory"));
    m_projectsDirectoryPathChooser = new PathChooser;
    m_closeSourceFilesCheckBox = new QCheckBox(tr("Close source files along with project"));
    m_saveAllFilesCheckBox = new QCheckBox(tr("Save all files before build"));
    m_deployProjectBeforeRunCheckBox = new QCheckBox(tr("Always deploy project before running it"));
    m_addLibraryPathsToRunEnvCheckBox =
            new QCheckBox(tr("Add linker library search paths to run environment"));
    m_promptToStopRunControlCheckBox = new QCheckBox(tr("Always ask before stopping applications"));
    m_automaticallyCreateRunConfiguration =
            new QCheckBox(tr("Create suitable run configurations automatically"));
    m_clearIssuesCheckBox = new QCheckBox(tr("Clear issues list on new build"));
    m_abortBuildAllOnErrorCheckBox = new QCheckBox(tr("Abort on error when building all projects"));
    m_lowBuildPriorityCheckBox = new QCheckBox(tr("Start build processes with low priority"));
    m_buildBeforeDeployComboBox = new QComboBox;
    m_buildBeforeDeployComboBox->addItem(tr("Do Not Build Anything"),
                                         int(BuildBeforeRunMode::Off));
    m_buildBeforeDeployComboBox->addItem(tr("Build the Whole Project"),
                                         int(BuildBeforeRunMode::WholeProject));
    m_buildBeforeDeployComboBox->addItem(tr("Build Only the Application to Be Run"),
                                         int(BuildBeforeRunMode::AppOnly));
    const QSizePolicy cbSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_buildBeforeDeployComboBox->setSizePolicy(cbSizePolicy);
    m_stopBeforeBuildComboBox = new QComboBox;
    m_stopBeforeBuildComboBox->addItem(tr("None"), int(StopBeforeBuild::None));
    m_stopBeforeBuildComboBox->addItem(tr("All"), int(StopBeforeBuild::All));
    m_stopBeforeBuildComboBox->addItem(tr("Same Project"), int(StopBeforeBuild::SameProject));
    m_stopBeforeBuildComboBox->addItem(tr("Same Build Directory"),
                                       int(StopBeforeBuild::SameBuildDir));
    m_stopBeforeBuildComboBox->addItem(tr("Same Application"),
                                       int(StopBeforeBuild::SameApp));
    m_stopBeforeBuildComboBox->setSizePolicy(cbSizePolicy);
    m_terminalModeComboBox = new QComboBox;
    m_terminalModeComboBox->addItem(tr("Enabled"));
    m_terminalModeComboBox->addItem(tr("Disabled"));
    m_terminalModeComboBox->addItem(tr("Deduced from Project"));
    m_terminalModeComboBox->setSizePolicy(cbSizePolicy);
    m_jomCheckbox = new QCheckBox(tr("Use jom instead of nmake"));
    auto jomLabel = new QLabel("<i>jom</i> is a drop-in replacement for <i>nmake</i> which "
                               "distributes the compilation process to multiple CPU cores. "
                               "The latest binary is available at "
                               "<a href=\"http://download.qt.io/official_releases/jom/\">"
                               "http://download.qt.io/official_releases/jom/</a>. "
                               "Disable it if you experience problems with your builds.");
    jomLabel->setWordWrap(true);

    using namespace Utils::Layouting;
    Column {
        Group {
            title(tr("Projects Directory")),
            Column {
                m_currentDirectoryRadioButton,
                Row { m_directoryRadioButton, m_projectsDirectoryPathChooser },
            },
        },
        Group {
            title(tr("Closing Projects")),
            Column {
                m_closeSourceFilesCheckBox,
            },
        },
        Group {
            title(tr("Build and Run")),
            Column {
                m_saveAllFilesCheckBox,
                m_deployProjectBeforeRunCheckBox,
                m_addLibraryPathsToRunEnvCheckBox,
                m_promptToStopRunControlCheckBox,
                m_automaticallyCreateRunConfiguration,
                m_clearIssuesCheckBox,
                m_abortBuildAllOnErrorCheckBox,
                m_lowBuildPriorityCheckBox,
                Form {
                    tr("Build before deploying:"), m_buildBeforeDeployComboBox, br,
                    tr("Stop applications before building:"), m_stopBeforeBuildComboBox, br,
                    tr("Default for \"Run in terminal\":"), m_terminalModeComboBox, br,
                },
                m_jomCheckbox,
                jomLabel,
            },
        },
        st,
    }.attachTo(this);

    m_jomCheckbox->setVisible(HostOsInfo::isWindowsHost());
    jomLabel->setVisible(HostOsInfo::isWindowsHost());

    m_directoryButtonGroup = new QButtonGroup;
    m_directoryButtonGroup->setExclusive(true);
    m_directoryButtonGroup->addButton(m_currentDirectoryRadioButton, UseCurrentDirectory);
    m_directoryButtonGroup->addButton(m_directoryRadioButton, UseProjectDirectory);

    connect(m_directoryButtonGroup, &QButtonGroup::buttonClicked,
            this, &ProjectExplorerSettingsWidget::slotDirectoryButtonGroupChanged);
}

ProjectExplorerSettings ProjectExplorerSettingsWidget::settings() const
{
    m_settings.buildBeforeDeploy = static_cast<BuildBeforeRunMode>(
                m_buildBeforeDeployComboBox->currentData().toInt());
    m_settings.deployBeforeRun = m_deployProjectBeforeRunCheckBox->isChecked();
    m_settings.saveBeforeBuild = m_saveAllFilesCheckBox->isChecked();
    m_settings.useJom = m_jomCheckbox->isChecked();
    m_settings.addLibraryPathsToRunEnv = m_addLibraryPathsToRunEnvCheckBox->isChecked();
    m_settings.prompToStopRunControl = m_promptToStopRunControlCheckBox->isChecked();
    m_settings.automaticallyCreateRunConfigurations = m_automaticallyCreateRunConfiguration->isChecked();
    m_settings.stopBeforeBuild = static_cast<StopBeforeBuild>(
                m_stopBeforeBuildComboBox->currentData().toInt());
    m_settings.terminalMode = static_cast<TerminalMode>(m_terminalModeComboBox->currentIndex());
    m_settings.closeSourceFilesWithProject = m_closeSourceFilesCheckBox->isChecked();
    m_settings.clearIssuesOnRebuild = m_clearIssuesCheckBox->isChecked();
    m_settings.abortBuildAllOnError = m_abortBuildAllOnErrorCheckBox->isChecked();
    m_settings.lowBuildPriority = m_lowBuildPriorityCheckBox->isChecked();
    return m_settings;
}

void ProjectExplorerSettingsWidget::setSettings(const ProjectExplorerSettings  &pes)
{
    m_settings = pes;
    m_buildBeforeDeployComboBox->setCurrentIndex(
                m_buildBeforeDeployComboBox->findData(int(m_settings.buildBeforeDeploy)));
    m_deployProjectBeforeRunCheckBox->setChecked(m_settings.deployBeforeRun);
    m_saveAllFilesCheckBox->setChecked(m_settings.saveBeforeBuild);
    m_jomCheckbox->setChecked(m_settings.useJom);
    m_addLibraryPathsToRunEnvCheckBox->setChecked(m_settings.addLibraryPathsToRunEnv);
    m_promptToStopRunControlCheckBox->setChecked(m_settings.prompToStopRunControl);
    m_automaticallyCreateRunConfiguration->setChecked(m_settings.automaticallyCreateRunConfigurations);
    m_stopBeforeBuildComboBox->setCurrentIndex(
                m_stopBeforeBuildComboBox->findData(int(m_settings.stopBeforeBuild)));
    m_terminalModeComboBox->setCurrentIndex(static_cast<int>(m_settings.terminalMode));
    m_closeSourceFilesCheckBox->setChecked(m_settings.closeSourceFilesWithProject);
    m_clearIssuesCheckBox->setChecked(m_settings.clearIssuesOnRebuild);
    m_abortBuildAllOnErrorCheckBox->setChecked(m_settings.abortBuildAllOnError);
    m_lowBuildPriorityCheckBox->setChecked(m_settings.lowBuildPriority);
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

// ------------------ ProjectExplorerSettingsPage
ProjectExplorerSettingsPage::ProjectExplorerSettingsPage()
{
    setId(Constants::BUILD_AND_RUN_SETTINGS_PAGE_ID);
    setDisplayName(ProjectExplorerSettingsWidget::tr("General"));
    setCategory(Constants::BUILD_AND_RUN_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer", "Build & Run"));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_buildrun.png");
}

QWidget *ProjectExplorerSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new ProjectExplorerSettingsWidget;
        m_widget->setSettings(ProjectExplorerPlugin::projectExplorerSettings());
        m_widget->setProjectsDirectory(DocumentManager::projectsDirectory());
        m_widget->setUseProjectsDirectory(DocumentManager::useProjectsDirectory());
    }
    return m_widget;
}

void ProjectExplorerSettingsPage::apply()
{
    if (m_widget) {
        ProjectExplorerPlugin::setProjectExplorerSettings(m_widget->settings());
        DocumentManager::setProjectsDirectory(m_widget->projectsDirectory());
        DocumentManager::setUseProjectsDirectory(m_widget->useProjectsDirectory());
    }
}

void ProjectExplorerSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace ProjectExplorer
