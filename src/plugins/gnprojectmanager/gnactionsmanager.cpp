// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnactionsmanager.h"

#include "gnbuildsystem.h"
#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"

#include <coreplugin/actionmanager/actionmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace GNProjectManager::Internal {

void setupGNActions(QObject *guard)
{
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

}

} // namespace GNProjectManager::Internal
