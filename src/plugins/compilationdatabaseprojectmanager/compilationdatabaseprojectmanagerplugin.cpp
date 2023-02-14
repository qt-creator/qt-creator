// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilationdatabaseprojectmanagerplugin.h"

#include "compilationdatabaseconstants.h"
#include "compilationdatabaseproject.h"
#include "compilationdatabaseprojectmanagertr.h"
#include "compilationdatabasetests.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectmanager.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/parameteraction.h>
#include <utils/utilsicons.h>

using namespace Core;
using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager::Internal {

const char CHANGEROOTDIR[] = "CompilationDatabaseProjectManager.ChangeRootDirectory";
const char COMPILE_COMMANDS_JSON[] = "compile_commands.json";

class CompilationDatabaseProjectManagerPluginPrivate
{
public:
    CompilationDatabaseEditorFactory editorFactory;
    CompilationDatabaseBuildConfigurationFactory buildConfigFactory;
    QAction changeRootAction{Tr::tr("Change Root Directory")};
};

CompilationDatabaseProjectManagerPlugin::~CompilationDatabaseProjectManagerPlugin()
{
    delete d;
}

void CompilationDatabaseProjectManagerPlugin::initialize()
{
    d = new CompilationDatabaseProjectManagerPluginPrivate;

    Utils::FileIconProvider::registerIconOverlayForFilename(Utils::Icons::PROJECT.imageFilePath().toString(),
                                                     COMPILE_COMMANDS_JSON);
    Utils::FileIconProvider::registerIconOverlayForFilename(
        Utils::Icons::PROJECT.imageFilePath().toString(),
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

    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
            this, onProjectChanged);

    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, onProjectChanged);

#ifdef WITH_TESTS
    addTest<CompilationDatabaseTests>();
#endif
}

} // CompilationDatabaseProjectManager::Internal
