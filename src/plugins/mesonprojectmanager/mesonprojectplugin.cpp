// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonactionsmanager.h"
#include "mesonbuildconfiguration.h"
#include "mesonbuildstep.h"
#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"
#include "mesonproject.h"
#include "mesonprojectmanagertr.h"
#include "mesonrunconfiguration.h"
#include "toolssettingsaccessor.h"
#include "toolssettingspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/fsengine/fileiconprovider.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

class MesonToolAspectFactory : public DeviceToolAspectFactory
{
public:
    MesonToolAspectFactory()
    {
        setToolId(Constants::ToolsSettings::TOOL_TYPE_MESON);
        setToolType(DeviceToolAspect::BuildTool);
        setFilePattern({"meson"});
        setLabelText(Tr::tr("Meson executable:"));
    }
};

void setupMesonTools()
{
    static MesonToolAspectFactory theMesonToolAspectFactory;
}

class MesonProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "MesonProjectManager.json")

    void initialize() final
    {
        Core::IOptionsPage::registerCategory(
            Constants::SettingsPage::CATEGORY, Tr::tr("Meson"), Constants::Icons::MESON_BW);

        setupMesonTools();

        setupToolsSettingsPage();
        setupToolsSettingsAccessor();

        setupMesonBuildSystem();
        setupMesonBuildConfiguration();
        setupMesonBuildStep();

        setupMesonRunConfiguration();
        setupMesonRunAndDebugWorkers();

        setupMesonProject();

        setupMesonActions(this);

        FileIconProvider::registerIconOverlayForFilename(Constants::Icons::MESON, "meson.build");
        FileIconProvider::registerIconOverlayForFilename(Constants::Icons::MESON, "meson_options.txt");
    }
};

} // MesonProjectManager::Internal

#include "mesonprojectplugin.moc"
