/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "projectexplorersettingspage.h"
#include "projectexplorersettings.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"

#include <QtGui/QLabel>

namespace ProjectExplorer {
namespace Internal {

ProjectExplorerSettingsWidget::ProjectExplorerSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
#ifndef Q_OS_WIN
    setJomVisible(false);
#endif
}

void ProjectExplorerSettingsWidget::setJomVisible(bool v)
{
    m_ui.jomCheckbox->setVisible(v);
    m_ui.jomLabel->setVisible(v);
}

ProjectExplorerSettings ProjectExplorerSettingsWidget::settings() const
{
    ProjectExplorerSettings pes;
    pes.buildBeforeRun = m_ui.buildProjectBeforeRunCheckBox->isChecked();
    pes.saveBeforeBuild = m_ui.saveAllFilesCheckBox->isChecked();
    pes.showCompilerOutput = m_ui.showCompileOutputCheckBox->isChecked();
    pes.useJom = m_ui.jomCheckbox->isChecked();
    return pes;
}

void ProjectExplorerSettingsWidget::setSettings(const ProjectExplorerSettings  &pes) const
{
    m_ui.buildProjectBeforeRunCheckBox->setChecked(pes.buildBeforeRun);
    m_ui.saveAllFilesCheckBox->setChecked(pes.saveBeforeBuild);
    m_ui.showCompileOutputCheckBox->setChecked(pes.showCompilerOutput);
    m_ui.jomCheckbox->setChecked(pes.useJom);
}

QString ProjectExplorerSettingsWidget::searchKeywords() const
{
    return QLatin1String("jom");
}

ProjectExplorerSettingsPage::ProjectExplorerSettingsPage()
{
}

QString ProjectExplorerSettingsPage::id() const
{
    return QLatin1String(Constants::PROJECTEXPLORER_PAGE);
}

QString ProjectExplorerSettingsPage::trName() const
{
    return tr("Build and Run");
}

QString ProjectExplorerSettingsPage::category() const
{
    return QLatin1String(Constants::PROJECTEXPLORER_PAGE);
}

QString ProjectExplorerSettingsPage::trCategory() const
{
    return tr("Projects");
}

QWidget *ProjectExplorerSettingsPage::createPage(QWidget *parent)
{
    m_widget = new ProjectExplorerSettingsWidget(parent);
    m_widget->setSettings(ProjectExplorerPlugin::instance()->projectExplorerSettings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void ProjectExplorerSettingsPage::apply()
{
    if (m_widget)
        ProjectExplorerPlugin::instance()->setProjectExplorerSettings(m_widget->settings());
}

void ProjectExplorerSettingsPage::finish()
{
    // Nothing to do
}

bool ProjectExplorerSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

}
}

