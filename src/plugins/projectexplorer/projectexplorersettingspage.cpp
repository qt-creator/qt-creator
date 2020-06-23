/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "projectexplorersettingspage.h"
#include "projectexplorersettings.h"
#include "projectexplorer.h"
#include "ui_projectexplorersettingspage.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>

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

    QString projectsDirectory() const;
    void setProjectsDirectory(const QString &pd);

    bool useProjectsDirectory();
    void setUseProjectsDirectory(bool v);

private:
    void slotDirectoryButtonGroupChanged();

    void setJomVisible(bool);

    Ui::ProjectExplorerSettingsPageUi m_ui;
    mutable ProjectExplorerSettings m_settings;
};

ProjectExplorerSettingsWidget::ProjectExplorerSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    setJomVisible(Utils::HostOsInfo::isWindowsHost());
    m_ui.stopBeforeBuildComboBox->addItem(tr("None"), int(StopBeforeBuild::None));
    m_ui.stopBeforeBuildComboBox->addItem(tr("All"), int(StopBeforeBuild::All));
    m_ui.stopBeforeBuildComboBox->addItem(tr("Same Project"), int(StopBeforeBuild::SameProject));
    m_ui.stopBeforeBuildComboBox->addItem(tr("Same Build Directory"),
                                          int(StopBeforeBuild::SameBuildDir));
    m_ui.stopBeforeBuildComboBox->addItem(tr("Same Application"),
                                          int(StopBeforeBuild::SameApp));
    m_ui.buildBeforeDeployComboBox->addItem(tr("Do Not Build Anything"),
                                            int(BuildBeforeRunMode::Off));
    m_ui.buildBeforeDeployComboBox->addItem(tr("Build the Whole Project"),
                                            int(BuildBeforeRunMode::WholeProject));
    m_ui.buildBeforeDeployComboBox->addItem(tr("Build Only the Application to Be Run"),
                                            int(BuildBeforeRunMode::AppOnly));
    m_ui.directoryButtonGroup->setId(m_ui.currentDirectoryRadioButton, UseCurrentDirectory);
    m_ui.directoryButtonGroup->setId(m_ui.directoryRadioButton, UseProjectDirectory);

    connect(m_ui.directoryButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &ProjectExplorerSettingsWidget::slotDirectoryButtonGroupChanged);
}

void ProjectExplorerSettingsWidget::setJomVisible(bool v)
{
    m_ui.jomCheckbox->setVisible(v);
    m_ui.jomLabel->setVisible(v);
}

ProjectExplorerSettings ProjectExplorerSettingsWidget::settings() const
{
    m_settings.buildBeforeDeploy = static_cast<BuildBeforeRunMode>(
                m_ui.buildBeforeDeployComboBox->currentData().toInt());
    m_settings.deployBeforeRun = m_ui.deployProjectBeforeRunCheckBox->isChecked();
    m_settings.saveBeforeBuild = m_ui.saveAllFilesCheckBox->isChecked();
    m_settings.useJom = m_ui.jomCheckbox->isChecked();
    m_settings.addLibraryPathsToRunEnv = m_ui.addLibraryPathsToRunEnvCheckBox->isChecked();
    m_settings.prompToStopRunControl = m_ui.promptToStopRunControlCheckBox->isChecked();
    m_settings.automaticallyCreateRunConfigurations = m_ui.automaticallyCreateRunConfiguration->isChecked();
    m_settings.stopBeforeBuild = static_cast<StopBeforeBuild>(
                m_ui.stopBeforeBuildComboBox->currentData().toInt());
    m_settings.terminalMode = static_cast<TerminalMode>(m_ui.terminalModeComboBox->currentIndex());
    m_settings.closeSourceFilesWithProject = m_ui.closeSourceFilesCheckBox->isChecked();
    m_settings.clearIssuesOnRebuild = m_ui.clearIssuesCheckBox->isChecked();
    m_settings.abortBuildAllOnError = m_ui.abortBuildAllOnErrorCheckBox->isChecked();
    m_settings.lowBuildPriority = m_ui.lowBuildPriorityCheckBox->isChecked();
    return m_settings;
}

void ProjectExplorerSettingsWidget::setSettings(const ProjectExplorerSettings  &pes)
{
    m_settings = pes;
    m_ui.buildBeforeDeployComboBox->setCurrentIndex(
                m_ui.buildBeforeDeployComboBox->findData(int(m_settings.buildBeforeDeploy)));
    m_ui.deployProjectBeforeRunCheckBox->setChecked(m_settings.deployBeforeRun);
    m_ui.saveAllFilesCheckBox->setChecked(m_settings.saveBeforeBuild);
    m_ui.jomCheckbox->setChecked(m_settings.useJom);
    m_ui.addLibraryPathsToRunEnvCheckBox->setChecked(m_settings.addLibraryPathsToRunEnv);
    m_ui.promptToStopRunControlCheckBox->setChecked(m_settings.prompToStopRunControl);
    m_ui.automaticallyCreateRunConfiguration->setChecked(m_settings.automaticallyCreateRunConfigurations);
    m_ui.stopBeforeBuildComboBox->setCurrentIndex(
                m_ui.stopBeforeBuildComboBox->findData(int(m_settings.stopBeforeBuild)));
    m_ui.terminalModeComboBox->setCurrentIndex(static_cast<int>(m_settings.terminalMode));
    m_ui.closeSourceFilesCheckBox->setChecked(m_settings.closeSourceFilesWithProject);
    m_ui.clearIssuesCheckBox->setChecked(m_settings.clearIssuesOnRebuild);
    m_ui.abortBuildAllOnErrorCheckBox->setChecked(m_settings.abortBuildAllOnError);
    m_ui.lowBuildPriorityCheckBox->setChecked(m_settings.lowBuildPriority);
}

QString ProjectExplorerSettingsWidget::projectsDirectory() const
{
    return m_ui.projectsDirectoryPathChooser->filePath().toString();
}

void ProjectExplorerSettingsWidget::setProjectsDirectory(const QString &pd)
{
    m_ui.projectsDirectoryPathChooser->setPath(pd);
}

bool ProjectExplorerSettingsWidget::useProjectsDirectory()
{
    return m_ui.directoryButtonGroup->checkedId() == UseProjectDirectory;
}

void ProjectExplorerSettingsWidget::setUseProjectsDirectory(bool b)
{
    if (useProjectsDirectory() != b) {
        (b ? m_ui.directoryRadioButton : m_ui.currentDirectoryRadioButton)->setChecked(true);
        slotDirectoryButtonGroupChanged();
    }
}

void ProjectExplorerSettingsWidget::slotDirectoryButtonGroupChanged()
{
    bool enable = useProjectsDirectory();
    m_ui.projectsDirectoryPathChooser->setEnabled(enable);
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
        m_widget->setProjectsDirectory(Core::DocumentManager::projectsDirectory().toString());
        m_widget->setUseProjectsDirectory(Core::DocumentManager::useProjectsDirectory());
    }
    return m_widget;
}

void ProjectExplorerSettingsPage::apply()
{
    if (m_widget) {
        ProjectExplorerPlugin::setProjectExplorerSettings(m_widget->settings());
        Core::DocumentManager::setProjectsDirectory(
            Utils::FilePath::fromString(m_widget->projectsDirectory()));
        Core::DocumentManager::setUseProjectsDirectory(m_widget->useProjectsDirectory());
    }
}

void ProjectExplorerSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace ProjectExplorer
