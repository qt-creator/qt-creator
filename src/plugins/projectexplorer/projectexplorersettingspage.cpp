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
#include <coreplugin/variablechooser.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>

namespace ProjectExplorer {
namespace Internal {

    enum { UseCurrentDirectory, UseProjectDirectory };

class ProjectExplorerSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectExplorerSettingsWidget(QWidget *parent = nullptr);

    ProjectExplorerSettings settings() const;
    void setSettings(const ProjectExplorerSettings  &s);

    QString projectsDirectory() const;
    void setProjectsDirectory(const QString &pd);

    bool useProjectsDirectory();
    void setUseProjectsDirectory(bool v);

    QString buildDirectory() const;
    void setBuildDirectory(const QString &bd);

private:
    void slotDirectoryButtonGroupChanged();
    void resetDefaultBuildDirectory();
    void updateResetButton();

    void setJomVisible(bool);

    Ui::ProjectExplorerSettingsPageUi m_ui;
    mutable ProjectExplorerSettings m_settings;
};

ProjectExplorerSettingsWidget::ProjectExplorerSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    setJomVisible(Utils::HostOsInfo::isWindowsHost());
    m_ui.directoryButtonGroup->setId(m_ui.currentDirectoryRadioButton, UseCurrentDirectory);
    m_ui.directoryButtonGroup->setId(m_ui.directoryRadioButton, UseProjectDirectory);

    connect(m_ui.directoryButtonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this, &ProjectExplorerSettingsWidget::slotDirectoryButtonGroupChanged);
    connect(m_ui.resetButton, &QAbstractButton::clicked,
            this, &ProjectExplorerSettingsWidget::resetDefaultBuildDirectory);
    connect(m_ui.buildDirectoryEdit, &QLineEdit::textChanged,
            this, &ProjectExplorerSettingsWidget::updateResetButton);

    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_ui.buildDirectoryEdit);
}

void ProjectExplorerSettingsWidget::setJomVisible(bool v)
{
    m_ui.jomCheckbox->setVisible(v);
    m_ui.jomLabel->setVisible(v);
}

ProjectExplorerSettings ProjectExplorerSettingsWidget::settings() const
{
    m_settings.buildBeforeDeploy = m_ui.buildProjectBeforeDeployCheckBox->isChecked();
    m_settings.deployBeforeRun = m_ui.deployProjectBeforeRunCheckBox->isChecked();
    m_settings.saveBeforeBuild = m_ui.saveAllFilesCheckBox->isChecked();
    m_settings.showCompilerOutput = m_ui.showCompileOutputCheckBox->isChecked();
    m_settings.showRunOutput = m_ui.showRunOutputCheckBox->isChecked();
    m_settings.showDebugOutput = m_ui.showDebugOutputCheckBox->isChecked();
    m_settings.cleanOldAppOutput = m_ui.cleanOldAppOutputCheckBox->isChecked();
    m_settings.mergeStdErrAndStdOut = m_ui.mergeStdErrAndStdOutCheckBox->isChecked();
    m_settings.wrapAppOutput = m_ui.wrapAppOutputCheckBox->isChecked();
    m_settings.useJom = m_ui.jomCheckbox->isChecked();
    m_settings.prompToStopRunControl = m_ui.promptToStopRunControlCheckBox->isChecked();
    m_settings.maxAppOutputLines = m_ui.maxAppOutputBox->value();
    m_settings.stopBeforeBuild = static_cast<ProjectExplorerSettings::StopBeforeBuild>(m_ui.stopBeforeBuildComboBox->currentIndex());
    return m_settings;
}

void ProjectExplorerSettingsWidget::setSettings(const ProjectExplorerSettings  &pes)
{
    m_settings = pes;
    m_ui.buildProjectBeforeDeployCheckBox->setChecked(m_settings.buildBeforeDeploy);
    m_ui.deployProjectBeforeRunCheckBox->setChecked(m_settings.deployBeforeRun);
    m_ui.saveAllFilesCheckBox->setChecked(m_settings.saveBeforeBuild);
    m_ui.showCompileOutputCheckBox->setChecked(m_settings.showCompilerOutput);
    m_ui.showRunOutputCheckBox->setChecked(m_settings.showRunOutput);
    m_ui.showDebugOutputCheckBox->setChecked(m_settings.showDebugOutput);
    m_ui.cleanOldAppOutputCheckBox->setChecked(m_settings.cleanOldAppOutput);
    m_ui.mergeStdErrAndStdOutCheckBox->setChecked(m_settings.mergeStdErrAndStdOut);
    m_ui.wrapAppOutputCheckBox->setChecked(m_settings.wrapAppOutput);
    m_ui.jomCheckbox->setChecked(m_settings.useJom);
    m_ui.promptToStopRunControlCheckBox->setChecked(m_settings.prompToStopRunControl);
    m_ui.maxAppOutputBox->setValue(m_settings.maxAppOutputLines);
    m_ui.stopBeforeBuildComboBox->setCurrentIndex(static_cast<int>(pes.stopBeforeBuild));
}

QString ProjectExplorerSettingsWidget::projectsDirectory() const
{
    return m_ui.projectsDirectoryPathChooser->path();
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

QString ProjectExplorerSettingsWidget::buildDirectory() const
{
    return m_ui.buildDirectoryEdit->text();
}

void ProjectExplorerSettingsWidget::setBuildDirectory(const QString &bd)
{
    m_ui.buildDirectoryEdit->setText(bd);
}

void ProjectExplorerSettingsWidget::slotDirectoryButtonGroupChanged()
{
    bool enable = useProjectsDirectory();
    m_ui.projectsDirectoryPathChooser->setEnabled(enable);
}

void ProjectExplorerSettingsWidget::resetDefaultBuildDirectory()
{
    setBuildDirectory(QLatin1String(Core::Constants::DEFAULT_BUILD_DIRECTORY));
}

void ProjectExplorerSettingsWidget::updateResetButton()
{
    m_ui.resetButton->setEnabled(buildDirectory() != QLatin1String(Core::Constants::DEFAULT_BUILD_DIRECTORY));
}

// ------------------ ProjectExplorerSettingsPage
ProjectExplorerSettingsPage::ProjectExplorerSettingsPage()
{
    setId(Constants::PROJECTEXPLORER_SETTINGS_ID);
    setDisplayName(tr("General"));
    setCategory(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *ProjectExplorerSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new ProjectExplorerSettingsWidget;
        m_widget->setSettings(ProjectExplorerPlugin::projectExplorerSettings());
        m_widget->setProjectsDirectory(Core::DocumentManager::projectsDirectory());
        m_widget->setUseProjectsDirectory(Core::DocumentManager::useProjectsDirectory());
        m_widget->setBuildDirectory(Core::DocumentManager::buildDirectory());
    }
    return m_widget;
}

void ProjectExplorerSettingsPage::apply()
{
    if (m_widget) {
        ProjectExplorerPlugin::setProjectExplorerSettings(m_widget->settings());
        Core::DocumentManager::setProjectsDirectory(m_widget->projectsDirectory());
        Core::DocumentManager::setUseProjectsDirectory(m_widget->useProjectsDirectory());
        Core::DocumentManager::setBuildDirectory(m_widget->buildDirectory());
    }
}

void ProjectExplorerSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace ProjectExplorer

#include "projectexplorersettingspage.moc"
