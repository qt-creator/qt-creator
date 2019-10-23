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

#include <utils/parameteraction.h>
#include <utils/utilsicons.h>

using namespace Core;
using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager {
namespace Internal {

const char CHANGEROOTDIR[] = "CompilationDatabaseProjectManager.ChangeRootDirectory";
const char COMPILE_COMMANDS_JSON[] = "compile_commands.json";

class CompilationDatabaseProjectManagerPluginPrivate
{
public:
    CompilationDatabaseEditorFactory editorFactory;
    CompilationDatabaseBuildConfigurationFactory buildConfigFactory;
    QAction changeRootAction{CompilationDatabaseProjectManagerPlugin::tr("Change Root Directory")};
};

CompilationDatabaseProjectManagerPlugin::~CompilationDatabaseProjectManagerPlugin()
{
    delete d;
}

bool CompilationDatabaseProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new CompilationDatabaseProjectManagerPluginPrivate;

    FileIconProvider::registerIconOverlayForFilename(
                Utils::Icons::PROJECT.imageFileName(),
                COMPILE_COMMANDS_JSON);
    FileIconProvider::registerIconOverlayForFilename(
                Utils::Icons::PROJECT.imageFileName(),
                QString(COMPILE_COMMANDS_JSON) + Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX);

    ProjectManager::registerProjectType<CompilationDatabaseProject>(
                Constants::COMPILATIONDATABASEMIMETYPE);

    Command *cmd = ActionManager::registerAction(&d->changeRootAction, CHANGEROOTDIR);

    ActionContainer *mprojectContextMenu = ActionManager::actionContainer(
        ProjectExplorer::Constants::M_PROJECTCONTEXT);
    mprojectContextMenu->addSeparator(ProjectExplorer::Constants::G_PROJECT_TREE);
    mprojectContextMenu->addAction(cmd, ProjectExplorer::Constants::G_PROJECT_TREE);

    connect(&d->changeRootAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::changeProjectRootDirectory);

    const auto onProjectChanged = [this] {
        const auto currentProject = qobject_cast<CompilationDatabaseProject *>(
                    ProjectTree::currentProject());

        d->changeRootAction.setEnabled(currentProject);
    };

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, onProjectChanged);

    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, onProjectChanged);

    return true;
}

QVector<QObject *> CompilationDatabaseProjectManagerPlugin::createTestObjects() const
{
    return {
#ifdef WITH_TESTS
        new CompilationDatabaseTests
#endif
    };
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
