/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "mesonactionsmanager.h"
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <project/mesonbuildsystem.h>
#include <project/projecttree/mesonprojectnodes.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>
#include <utils/parameteraction.h>
namespace MesonProjectManager {
namespace Internal {

MesonActionsManager::MesonActionsManager()
    : configureActionMenu(tr("Configure"))
    , configureActionContextMenu(tr("Configure"))
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
