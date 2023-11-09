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

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

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

    ActionBuilder editAction(this, "GenericProjectManager.EditFiles");
    editAction.setContext(Constants::GENERICPROJECT_ID);
    editAction.setText(Tr::tr("Edit Files..."));
    editAction.setCommandAttribute(Command::CA_Hide);
    editAction.setContainer(PEC::M_PROJECTCONTEXT, PEC::G_PROJECT_FILES);
    editAction.setOnTriggered([] {
        if (auto genericProject = qobject_cast<GenericProject *>(ProjectTree::currentProject()))
            genericProject->editFilesTriggered();
    });

    ActionBuilder removeDirAction(this, "GenericProject.RemoveDir");
    removeDirAction.setContext(PEC::C_PROJECT_TREE);
    removeDirAction.setText(Tr::tr("Remove Directory"));
    removeDirAction.setContainer(PEC::M_FOLDERCONTEXT, PEC::G_FOLDER_OTHER);
    removeDirAction.setOnTriggered([] {
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
