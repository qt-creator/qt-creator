/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakebuildsettingswidget.h"
#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildinfo.h"
#include "cmakeopenprojectwizard.h"
#include "cmakeprojectmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <QFormLayout>

namespace CMakeProjectManager {
namespace Internal {

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) : m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(20, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);

    QPushButton *runCmakeButton = new QPushButton(tr("Run CMake..."));
    connect(runCmakeButton, &QAbstractButton::clicked, this, &CMakeBuildSettingsWidget::runCMake);
    fl->addRow(tr("Reconfigure project:"), runCmakeButton);

    m_pathLineEdit = new QLineEdit(this);
    m_pathLineEdit->setReadOnly(true);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addWidget(m_pathLineEdit);

    m_changeButton = new QPushButton(this);
    m_changeButton->setText(tr("&Change"));
    connect(m_changeButton, &QAbstractButton::clicked, this,
            &CMakeBuildSettingsWidget::openChangeBuildDirectoryDialog);
    hbox->addWidget(m_changeButton);

    fl->addRow(tr("Build directory:"), hbox);

    m_buildConfiguration = bc;
    m_pathLineEdit->setText(m_buildConfiguration->rawBuildDirectory().toString());
    if (m_buildConfiguration->buildDirectory() == bc->target()->project()->projectDirectory())
        m_changeButton->setEnabled(false);
    else
        m_changeButton->setEnabled(true);

    setDisplayName(tr("CMake"));
}

void CMakeBuildSettingsWidget::openChangeBuildDirectoryDialog()
{
    CMakeProject *project = static_cast<CMakeProject *>(m_buildConfiguration->target()->project());
    CMakeManager *manager = static_cast<CMakeManager *>(project->projectManager());
    CMakeBuildInfo info(m_buildConfiguration);
    CMakeOpenProjectWizard copw(Core::ICore::mainWindow(),
                                manager, CMakeOpenProjectWizard::ChangeDirectory,
                                &info,
                                project->activeTarget()->displayName(),
                                project->activeTarget()->activeBuildConfiguration()->displayName());
    if (copw.exec() == QDialog::Accepted) {
        project->changeBuildDirectory(m_buildConfiguration, copw.buildDirectory());
        m_buildConfiguration->setUseNinja(copw.useNinja());
        m_pathLineEdit->setText(m_buildConfiguration->rawBuildDirectory().toString());
    }
}

void CMakeBuildSettingsWidget::runCMake()
{
    if (!ProjectExplorer::ProjectExplorerPlugin::saveModifiedFiles())
        return;
    CMakeProject *project = static_cast<CMakeProject *>(m_buildConfiguration->target()->project());
    CMakeManager *manager = static_cast<CMakeManager *>(project->projectManager());
    CMakeBuildInfo info(m_buildConfiguration);
    CMakeOpenProjectWizard copw(Core::ICore::mainWindow(),
                                manager,
                                CMakeOpenProjectWizard::WantToUpdate, &info,
                                project->activeTarget()->displayName(),
                                project->activeTarget()->activeBuildConfiguration()->displayName());
    if (copw.exec() == QDialog::Accepted)
        project->parseCMakeLists();
}
} // namespace Internal
} // namespace CMakeProjectManager
