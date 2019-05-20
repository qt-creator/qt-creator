/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "clangindexingprojectsettingswidget.h"
#include "ui_clangindexingprojectsettingswidget.h"

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/project.h>

#include "pchmanagerprojectupdater.h"
#include "preprocessormacrocollector.h"
#include "preprocessormacrowidget.h"
#include "qtcreatorprojectupdater.h"

namespace ClangPchManager {
ClangIndexingProjectSettingsWidget::ClangIndexingProjectSettingsWidget(
    ClangIndexingProjectSettings *settings,
    ProjectExplorer::Project *project,
    QtCreatorProjectUpdater<PchManagerProjectUpdater> &projectUpdater)
    : ui(new Ui::ClangIndexingProjectSettingsWidget)
    , m_project(project)
    , m_projectUpdater(projectUpdater)
{
    ui->setupUi(this);
    ui->preprocessorMacrosWidget->setSettings(settings);
    connect(ui->reindexButton, &QPushButton::clicked, this, &ClangIndexingProjectSettingsWidget::reindex);
}

ClangIndexingProjectSettingsWidget::~ClangIndexingProjectSettingsWidget()
{
    delete ui;
}

void ClangIndexingProjectSettingsWidget::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    const CppTools::ProjectInfo projectInfo = CppTools::CppModelManager::instance()->projectInfo(
        project);

    PreprocessorMacroCollector collector;

    for (auto projectPart : projectInfo.projectParts())
        collector.add(projectPart->projectMacros);

    ui->preprocessorMacrosWidget->setBasePreprocessorMacros(collector.macros());
}

void ClangIndexingProjectSettingsWidget::reindex()
{
    m_projectUpdater.projectPartsUpdated(m_project);
}

} // namespace ClangPchManager
