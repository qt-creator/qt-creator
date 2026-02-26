// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnactionsmanager.h"

#include "gnbuildsystem.h"
#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"
#include "gnprojectnodes.h"

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

namespace GNProjectManager::Internal {

void setupGNActions(QObject *guard)
{
    static Action *buildTargetContextAction = nullptr;

    const Context projectContext{Constants::GN_PROJECT_ID};

    ActionBuilder(guard, "GNProject.Generate")
        .setText(Tr::tr("Generate"))
        .setContext(projectContext)
        .addToContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT,
                        ProjectExplorer::Constants::G_PROJECT_BUILD)
        .addToContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT,
                        ProjectExplorer::Constants::G_PROJECT_BUILD)
        .addOnTriggered(guard, [] {
            auto bs = dynamic_cast<GNBuildSystem *>(activeBuildSystemForCurrentProject());
            QTC_ASSERT(bs, return);
            if (ProjectExplorerPlugin::saveModifiedFiles())
                bs->generate();
        });

    ActionBuilder(guard, "GN.BuildTargetContextMenu")
        .setParameterText(Tr::tr("Build \"%1\""), Tr::tr("Build"), ActionBuilder::AlwaysEnabled)
        .bindContextAction(&buildTargetContextAction)
        .setContext(projectContext)
        .setCommandAttribute(Command::CA_Hide)
        .setCommandAttribute(Command::CA_UpdateText)
        .setCommandDescription(Tr::tr("Build"))
        .addToContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT,
                        ProjectExplorer::Constants::G_PROJECT_BUILD)
        .addOnTriggered(guard, [] {
            if (qobject_cast<GNBuildSystem *>(activeBuildSystemForCurrentProject())) {
                auto targetNode = dynamic_cast<GNTargetNode *>(ProjectTree::currentNode());
                targetNode->build();
            }
        });

    QObject::connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged, guard, [&] {
        auto targetNode = dynamic_cast<const GNTargetNode *>(ProjectTree::currentNode());
        const QString targetDisplayName = targetNode ? targetNode->displayName() : QString();

        buildTargetContextAction->setParameter(targetDisplayName);
        buildTargetContextAction->setEnabled(targetNode);
        buildTargetContextAction->setVisible(targetNode);
    });
}

} // namespace GNProjectManager::Internal
