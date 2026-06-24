// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonactionsmanager.h"

#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"

#include <coreplugin/actionmanager/actionmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

void setupMesonActions(QObject *guard)
{
    const Context projectContext{Constants::Project::ID};
    ActionBuilder(guard, "MesonProject.Configure")
        .setText(Tr::tr("Configure"))
        .setContext(projectContext)
        .addToContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT,
                        ProjectExplorer::Constants::G_PROJECT_BUILD)
        .addToContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT,
                        ProjectExplorer::Constants::G_PROJECT_BUILD)
        .addOnTriggered(guard, [] {
            auto bs = dynamic_cast<MesonBuildSystem *>(activeBuildSystemForCurrentProject());
            QTC_ASSERT(bs, return);
            if (ProjectExplorerPlugin::saveModifiedFiles())
                bs->configure();
        });
}

} // MesonProjectManager::Internal
