// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonactionsmanager.h"

#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesonprojectnodes.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>

#include <utils/action.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

void setupMesonActions(QObject *guard)
{
    static Action *buildTargetContextAction = nullptr;

    const Context projectContext{Constants::Project::ID};

    ActionBuilder(guard, "MesonProject.Configure")
        .setText(Tr::tr("Configure"))
        .setContext(projectContext)
        .addToContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT,
                        ProjectExplorer::Constants::G_PROJECT_BUILD)
        .addToContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT,
                        ProjectExplorer::Constants::G_PROJECT_BUILD)
        .addOnTriggered(guard, [] {
            auto bs = dynamic_cast<MesonBuildSystem *>(ProjectTree::currentBuildSystem());
            QTC_ASSERT(bs, return);
            if (ProjectExplorerPlugin::saveModifiedFiles())
                bs->configure();
        });

    ActionBuilder(guard, "Meson.BuildTargetContextMenu")
        .setParameterText(Tr::tr("Build \"%1\""), Tr::tr("Build"),
                          ActionBuilder::AlwaysEnabled /*handled manually*/)
        .bindContextAction(&buildTargetContextAction)
        .setContext(projectContext)
        .setCommandAttribute(Core::Command::CA_Hide)
        .setCommandAttribute(Core::Command::CA_UpdateText)
        .setCommandDescription(Tr::tr("Build"))
        .addToContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT,
                        ProjectExplorer::Constants::G_PROJECT_BUILD)
        .addOnTriggered(guard, [] {
            if (qobject_cast<MesonBuildSystem *>(ProjectTree::currentBuildSystem())) {
                auto targetNode = dynamic_cast<MesonTargetNode *>(ProjectTree::currentNode());
                targetNode->build();
            }
        });

    QObject::connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged, guard, [&] {
        auto targetNode = dynamic_cast<const MesonTargetNode *>(ProjectTree::currentNode());
        const QString targetDisplayName = targetNode ? targetNode->displayName() : QString();

        buildTargetContextAction->setParameter(targetDisplayName);
        buildTargetContextAction->setEnabled(targetNode);
        buildTargetContextAction->setVisible(targetNode);
    });
}

} // MesonProjectManager::Internal
