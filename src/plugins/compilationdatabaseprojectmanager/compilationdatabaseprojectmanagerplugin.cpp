// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilationdatabaseconstants.h"
#include "compilationdatabaseproject.h"
#include "compilationdatabaseprojectmanagertr.h"
#include "compilationdatabasetests.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectmanager.h>

#include <utils/action.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/utilsicons.h>

using namespace Core;
using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager::Internal {

const char CHANGEROOTDIR[] = "CompilationDatabaseProjectManager.ChangeRootDirectory";
const char COMPILE_COMMANDS_JSON[] = "compile_commands.json";

class CompilationDatabaseProjectManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CompilationDatabaseProjectManager.json")

    void initialize() final
    {
        setupCompilationDatabaseEditor();
        setupCompilationDatabaseBuildConfiguration();

        Utils::FileIconProvider::registerIconOverlayForFilename(Utils::Icons::PROJECT.imageFilePath().toString(),
                                                                COMPILE_COMMANDS_JSON);
        Utils::FileIconProvider::registerIconOverlayForFilename(
            Utils::Icons::PROJECT.imageFilePath().toString(),
            QString(COMPILE_COMMANDS_JSON) + Constants::COMPILATIONDATABASEPROJECT_FILES_SUFFIX);

        ProjectManager::registerProjectType<CompilationDatabaseProject>(
            Constants::COMPILATIONDATABASEMIMETYPE);

        ActionContainer *mprojectContextMenu = ActionManager::actionContainer(
            ProjectExplorer::Constants::M_PROJECTCONTEXT);
        mprojectContextMenu->addSeparator(ProjectExplorer::Constants::G_PROJECT_TREE);

        QAction *changeRootAction = nullptr;
        ActionBuilder(this, CHANGEROOTDIR)
            .setText(Tr::tr("Change Root Directory"))
            .bindContextAction(&changeRootAction)
            .addToContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT)
            .addOnTriggered(ProjectTree::instance(), &ProjectTree::changeProjectRootDirectory);

        const auto onProjectChanged = [changeRootAction] {
            const auto currentProject = qobject_cast<CompilationDatabaseProject *>(
                ProjectTree::currentProject());
            changeRootAction->setEnabled(currentProject);
        };

        connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
                this, onProjectChanged);

        connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
                this, onProjectChanged);

#ifdef WITH_TESTS
        addTest<CompilationDatabaseTests>();
#endif
    }
};

} // CompilationDatabaseProjectManager::Internal

#include "compilationdatabaseprojectmanagerplugin.moc"
