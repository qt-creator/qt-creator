/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectexplorersettingspage.h"
#include "projectexplorersettings.h"
#include "projectexplorer.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/variablechooser.h>
#include <utils/hostosinfo.h>

#include <QCoreApplication>

namespace ProjectExplorer {
namespace Internal {

    enum { UseCurrentDirectory, UseProjectDirectory };

ProjectExplorerSettingsWidget::ProjectExplorerSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    new Core::VariableChooser(this);
    Core::VariableChooser::addVariableSupport(m_ui.buildDirectoryEdit);
    setJomVisible(Utils::HostOsInfo::isWindowsHost());
    m_ui.directoryButtonGroup->setId(m_ui.currentDirectoryRadioButton, UseCurrentDirectory);
    m_ui.directoryButtonGroup->setId(m_ui.directoryRadioButton, UseProjectDirectory);

    connect(m_ui.directoryButtonGroup, SIGNAL(buttonClicked(int)),
            this, SLOT(slotDirectoryButtonGroupChanged()));
    connect(m_ui.resetButton, SIGNAL(clicked()), this, SLOT(resetDefaultBuildDirectory()));
    connect(m_ui.buildDirectoryEdit, SIGNAL(textChanged(QString)), this, SLOT(updateResetButton()));
}

void ProjectExplorerSettingsWidget::setJomVisible(bool v)
{
    m_ui.jomCheckbox->setVisible(v);
    m_ui.jomLabel->setVisible(v);
}

ProjectExplorerSettings ProjectExplorerSettingsWidget::settings() const
{
    ProjectExplorerSettings pes;
    pes.buildBeforeDeploy = m_ui.buildProjectBeforeDeployCheckBox->isChecked();
    pes.deployBeforeRun = m_ui.deployProjectBeforeRunCheckBox->isChecked();
    pes.saveBeforeBuild = m_ui.saveAllFilesCheckBox->isChecked();
    pes.showCompilerOutput = m_ui.showCompileOutputCheckBox->isChecked();
    pes.showRunOutput = m_ui.showRunOutputCheckBox->isChecked();
    pes.showDebugOutput = m_ui.showDebugOutputCheckBox->isChecked();
    pes.cleanOldAppOutput = m_ui.cleanOldAppOutputCheckBox->isChecked();
    pes.mergeStdErrAndStdOut = m_ui.mergeStdErrAndStdOutCheckBox->isChecked();
    pes.wrapAppOutput = m_ui.wrapAppOutputCheckBox->isChecked();
    pes.useJom = m_ui.jomCheckbox->isChecked();
    pes.prompToStopRunControl = m_ui.promptToStopRunControlCheckBox->isChecked();
    pes.maxAppOutputLines = m_ui.maxAppOutputBox->value();
    return pes;
}

void ProjectExplorerSettingsWidget::setSettings(const ProjectExplorerSettings  &pes) const
{
    m_ui.buildProjectBeforeDeployCheckBox->setChecked(pes.buildBeforeDeploy);
    m_ui.deployProjectBeforeRunCheckBox->setChecked(pes.deployBeforeRun);
    m_ui.saveAllFilesCheckBox->setChecked(pes.saveBeforeBuild);
    m_ui.showCompileOutputCheckBox->setChecked(pes.showCompilerOutput);
    m_ui.showRunOutputCheckBox->setChecked(pes.showRunOutput);
    m_ui.showDebugOutputCheckBox->setChecked(pes.showDebugOutput);
    m_ui.cleanOldAppOutputCheckBox->setChecked(pes.cleanOldAppOutput);
    m_ui.mergeStdErrAndStdOutCheckBox->setChecked(pes.mergeStdErrAndStdOut);
    m_ui.wrapAppOutputCheckBox->setChecked(pes.wrapAppOutput);
    m_ui.jomCheckbox->setChecked(pes.useJom);
    m_ui.promptToStopRunControlCheckBox->setChecked(pes.prompToStopRunControl);
    m_ui.maxAppOutputBox->setValue(pes.maxAppOutputLines);
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

QString ProjectExplorerSettingsWidget::searchKeywords() const
{
    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        m_searchKeywords = m_ui.directoryGroupBox->title()
                + sep + m_ui.currentDirectoryRadioButton->text()
                + sep + m_ui.directoryRadioButton->text()
                + sep + m_ui.buildAndRunGroupBox->title()
                + sep + m_ui.saveAllFilesCheckBox->text()
                + sep + m_ui.buildProjectBeforeDeployCheckBox->text()
                + sep + m_ui.deployProjectBeforeRunCheckBox->text()
                + sep + m_ui.showCompileOutputCheckBox->text()
                + sep + m_ui.cleanOldAppOutputCheckBox->text()
                + sep + m_ui.mergeStdErrAndStdOutCheckBox->text()
                + sep + m_ui.mergeStdErrAndStdOutCheckBox->toolTip()
                + sep + m_ui.wrapAppOutputCheckBox->text()
                + sep + m_ui.jomLabel->text()
                ;
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return m_searchKeywords;
}

// ------------------ ProjectExplorerSettingsPage
ProjectExplorerSettingsPage::ProjectExplorerSettingsPage()
{
    setId(Constants::PROJECTEXPLORER_SETTINGS_ID);
    setDisplayName(tr("General"));
    setCategory(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

ProjectExplorerSettingsPage::~ProjectExplorerSettingsPage()
{
}

QWidget *ProjectExplorerSettingsPage::createPage(QWidget *parent)
{
    m_widget = new ProjectExplorerSettingsWidget(parent);
    m_widget->setSettings(ProjectExplorerPlugin::instance()->projectExplorerSettings());
    m_widget->setProjectsDirectory(Core::DocumentManager::projectsDirectory());
    m_widget->setUseProjectsDirectory(Core::DocumentManager::useProjectsDirectory());
    m_widget->setBuildDirectory(Core::DocumentManager::buildDirectory());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void ProjectExplorerSettingsPage::apply()
{
    if (m_widget) {
        ProjectExplorerPlugin::instance()->setProjectExplorerSettings(m_widget->settings());
        Core::DocumentManager::setProjectsDirectory(m_widget->projectsDirectory());
        Core::DocumentManager::setUseProjectsDirectory(m_widget->useProjectsDirectory());
        Core::DocumentManager::setBuildDirectory(m_widget->buildDirectory());
    }
}

void ProjectExplorerSettingsPage::finish()
{
    // Nothing to do
}

bool ProjectExplorerSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace ProjectExplorer

