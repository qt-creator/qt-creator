// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonactionsmanager.h"

#include "mesonbuildsystem.h"
#include "mesonprojectmanagertr.h"
#include "mesonprojectnodes.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>

#include <utils/parameteraction.h>

namespace MesonProjectManager {
namespace Internal {

MesonActionsManager::MesonActionsManager()
    : configureActionMenu(Tr::tr("Configure"))
    , configureActionContextMenu(Tr::tr("Configure"))
{
    const Core::Context globalContext(Core::Constants::C_GLOBAL);
    const Core::Context projectContext{Constants::Project::ID};
    Core::ActionContainer *mproject = Core::ActionManager::actionContainer(
        ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject = Core::ActionManager::actionContainer(
        ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    Core::Command *command;

    {
        command = Core::ActionManager::registerAction(&configureActionMenu,
                                                      "MesonProject.Configure",
                                                      projectContext);
        mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
        msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
        connect(&configureActionMenu,
                &QAction::triggered,
                this,
                &MesonActionsManager::configureCurrentProject);
    }

    {
        command = Core::ActionManager::registerAction(&buildTargetContextAction,
                                                      "Meson.BuildTargetContextMenu",
                                                      projectContext);
        command->setAttribute(Core::Command::CA_Hide);
        command->setAttribute(Core::Command::CA_UpdateText);
        command->setDescription(buildTargetContextAction.text());

        Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT)
            ->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
        // Wire up context menu updates:
        connect(ProjectExplorer::ProjectTree::instance(),
                &ProjectExplorer::ProjectTree::currentNodeChanged,
                this,
                &MesonActionsManager::updateContextActions);

        connect(&buildTargetContextAction, &Utils::ParameterAction::triggered, this, [] {
            auto bs = qobject_cast<MesonBuildSystem *>(
                ProjectExplorer::ProjectTree::currentBuildSystem());
            if (bs) {
                auto targetNode = dynamic_cast<MesonTargetNode *>(
                    ProjectExplorer::ProjectTree::currentNode());
                targetNode->build();
            }
        });
    }
}

void MesonActionsManager::configureCurrentProject()
{
    auto bs = dynamic_cast<MesonBuildSystem *>(ProjectExplorer::ProjectTree::currentBuildSystem());
    QTC_ASSERT(bs, return );
    if (ProjectExplorer::ProjectExplorerPlugin::saveModifiedFiles())
        bs->configure();
}

void MesonActionsManager::updateContextActions()
{
    auto targetNode = dynamic_cast<const MesonTargetNode *>(
        ProjectExplorer::ProjectTree::currentNode());
    const QString targetDisplayName = targetNode ? targetNode->displayName() : QString();

    // Build Target:
    buildTargetContextAction.setParameter(targetDisplayName);
    buildTargetContextAction.setEnabled(targetNode);
    buildTargetContextAction.setVisible(targetNode);
}

} // namespace Internal
} // namespace MesonProjectManager
