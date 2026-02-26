// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnactionsmanager.h"
#include "gnbuildconfiguration.h"
#include "gnbuildstep.h"
#include "gnkitaspect.h"
#include "gnpluginconstants.h"
#include "gnproject.h"
#include "gnprojectmanagertr.h"
#include "gntoolssettingsaccessor.h"
#include "gntoolssettingspage.h"

#include <extensionsystem/iplugin.h>

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/fsengine/fileiconprovider.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace GNProjectManager::Internal {

class GNProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GNProjectManager.json")

    void initialize() final
    {
        Core::IOptionsPage::registerCategory(Constants::SettingsPage::CATEGORY,
                                             Tr::tr("GN"),
                                             ":/projectexplorer/images/build.png");

        setupGNBuildConfiguration();
        setupGNBuildStep();

        setupGNToolsSettingsAccessor();
        setupGNToolsSettingsPage();
        setupGNKitAspect();

        setupGNProject();
        setupGNActions(this);

        FileIconProvider::
            registerIconOverlayForFilename(":/projectexplorer/images/build.png", "BUILD.gn");
        FileIconProvider::
            registerIconOverlayForFilename(":/projectexplorer/images/build.png", ".gn");
    }
};

} // namespace GNProjectManager::Internal

#include "gnprojectplugin.moc"
