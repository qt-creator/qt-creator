/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "compilationdatabaseprojectmanagerplugin.h"

#include "compilationdatabaseconstants.h"
#include "compilationdatabaseproject.h"
#include "compilationdatabasetests.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <utils/utilsicons.h>

namespace CompilationDatabaseProjectManager {
namespace Internal {

const char CHANGEROOTDIR[] = "CompilationDatabaseProjectManager.ChangeRootDirectory";
const char COMPILE_COMMANDS_JSON[] = "compile_commands.json";

bool CompilationDatabaseProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    Core::FileIconProvider::registerIconOverlayForFilename(
                Utils::Icons::PROJECT.imageFileName(),
                COMPILE_COMMANDS_JSON);
    Core::FileIconProvider::registerIconOverlayForFilename(
                Utils::Icons::PROJECT.imageFileName(),
                QString(COMPILE_COMMANDS_JSON) + Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX);

    ProjectExplorer::ProjectManager::registerProjectType<CompilationDatabaseProject>(
                Constants::COMPILATIONDATABASEMIMETYPE);
    m_changeProjectRootDirectoryAction = new QAction(tr("Change Root Directory"), this);
    Core::Command *cmd = Core::ActionManager::registerAction(m_changeProjectRootDirectoryAction,
                                                             CHANGEROOTDIR);
    Core::ActionContainer *mprojectContextMenu = Core::ActionManager::actionContainer(
        ProjectExplorer::Constants::M_PROJECTCONTEXT);
    mprojectContextMenu->addSeparator(ProjectExplorer::Constants::G_PROJECT_TREE);
    mprojectContextMenu->addAction(cmd, ProjectExplorer::Constants::G_PROJECT_TREE);

    connect(m_changeProjectRootDirectoryAction,
            &QAction::triggered,
            ProjectExplorer::ProjectTree::instance(),
            &ProjectExplorer::ProjectTree::changeProjectRootDirectory);

    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged,
            this,
            &CompilationDatabaseProjectManagerPlugin::projectChanged);
    connect(ProjectExplorer::ProjectTree::instance(),
            &ProjectExplorer::ProjectTree::currentProjectChanged,
            this,
            &CompilationDatabaseProjectManagerPlugin::projectChanged);

    return true;
}

void CompilationDatabaseProjectManagerPlugin::projectChanged()
{
    const auto *currentProject = qobject_cast<CompilationDatabaseProject *>(
        ProjectExplorer::ProjectTree::currentProject());

    m_changeProjectRootDirectoryAction->setEnabled(currentProject);
}

void CompilationDatabaseProjectManagerPlugin::extensionsInitialized()
{
}

QVector<QObject *> CompilationDatabaseProjectManagerPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#ifdef WITH_TESTS
    tests << new CompilationDatabaseTests;
#endif
    return tests;
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
