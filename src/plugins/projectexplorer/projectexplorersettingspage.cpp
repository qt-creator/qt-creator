/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "projectexplorersettingspage.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"

#include <QtGui/QLabel>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ProjectExplorerSettingsPage::ProjectExplorerSettingsPage()
{

}
ProjectExplorerSettingsPage::~ProjectExplorerSettingsPage()
{

}

QString ProjectExplorerSettingsPage::id() const
{
    return Constants::PROJECTEXPLORER_PAGE;
}

QString ProjectExplorerSettingsPage::trName() const
{
    return tr("Build and Run Settings");
}

QString ProjectExplorerSettingsPage::category() const
{
    return Constants::PROJECTEXPLORER_PAGE;
}

QString ProjectExplorerSettingsPage::trCategory() const
{
    return tr("Projectexplorer");
}

QWidget *ProjectExplorerSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    ProjectExplorerSettings pes = ProjectExplorerPlugin::instance()->projectExplorerSettings();
    m_ui.buildProjectBeforeRunCheckBox->setChecked(pes.buildBeforeRun);
    m_ui.saveAllFilesCheckBox->setChecked(pes.saveBeforeBuild);
    return w;
}

void ProjectExplorerSettingsPage::apply()
{
    ProjectExplorerSettings pes;
    pes.buildBeforeRun = m_ui.buildProjectBeforeRunCheckBox->isChecked();
    pes.saveBeforeBuild = m_ui.saveAllFilesCheckBox->isChecked();
    ProjectExplorerPlugin::instance()->setProjectExplorerSettings(pes);
}

void ProjectExplorerSettingsPage::finish()
{
    // Nothing to do
}
