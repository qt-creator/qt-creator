// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericprojectplugin.h"

#include "genericbuildconfiguration.h"
#include "genericmakestep.h"
#include "genericproject.h"
#include "genericprojectconstants.h"
#include "genericprojectfileseditor.h"
#include "genericprojectmanagertr.h"
#include "genericprojectwizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QAction>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;
namespace PEC = ProjectExplorer::Constants;

namespace GenericProjectManager {
namespace Internal {

class GenericProjectPluginPrivate : public QObject
{
public:
    GenericProjectPluginPrivate();

    ProjectFilesFactory projectFilesFactory;
    GenericMakeStepFactory makeStepFactory;
    GenericBuildConfigurationFactory buildConfigFactory;

    QAction editFilesAction{Tr::tr("Edit Files..."), nullptr};
};

GenericProjectPlugin::~GenericProjectPlugin()
{
    delete d;
}

void GenericProjectPlugin::initialize()
{
    d = new GenericProjectPluginPrivate;
}

GenericProjectPluginPrivate::GenericProjectPluginPrivate()
{
    ProjectManager::registerProjectType<GenericProject>(Constants::GENERICMIMETYPE);

    IWizardFactory::registerFactoryCreator([] { return new GenericProjectWizard; });

    ActionContainer *mproject = ActionManager::actionContainer(PEC::M_PROJECTCONTEXT);

    Command *command = ActionManager::registerAction(&editFilesAction,
        "GenericProjectManager.EditFiles", Context(Constants::GENERICPROJECT_ID));
    command->setAttribute(Command::CA_Hide);
    mproject->addAction(command, PEC::G_PROJECT_FILES);

    connect(&editFilesAction, &QAction::triggered, this, [] {
        if (auto genericProject = qobject_cast<GenericProject *>(ProjectTree::currentProject()))
            genericProject->editFilesTriggered();
    });

    const auto removeDirAction = new QAction(Tr::tr("Remove Directory"), this);
    Command * const cmd = ActionManager::registerAction(removeDirAction, "GenericProject.RemoveDir",
                                                        Context(PEC::C_PROJECT_TREE));
    ActionManager::actionContainer(PEC::M_FOLDERCONTEXT)->addAction(cmd, PEC::G_FOLDER_OTHER);
    connect(removeDirAction, &QAction::triggered, this, [] {
        const auto folderNode = ProjectTree::currentNode()->asFolderNode();
        QTC_ASSERT(folderNode, return);
        const auto project = qobject_cast<GenericProject *>(folderNode->getProject());
        QTC_ASSERT(project, return);
        const FilePaths filesToRemove = transform(
                    folderNode->findNodes([](const Node *node) { return node->asFileNode(); }),
                    [](const Node *node) { return node->filePath();});
        project->removeFilesTriggered(filesToRemove);
    });
}

} // namespace Internal
} // namespace GenericProjectManager
