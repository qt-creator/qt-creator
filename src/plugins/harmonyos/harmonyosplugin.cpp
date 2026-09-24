// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosbuilddevice.h"
#include "harmonyosconstants.h"
#include "harmonyosconfigurations.h"
#include "harmonyosdebugsupport.h"
#include "harmonyosdeploystep.h"
#include "harmonyosdevice.h"
#include "harmonyosqtversion.h"
#include "harmonyosrunconfiguration.h"
#include "harmonyossdk.h"
#include "harmonyossettings.h"
#include "harmonyostoolchain.h"

#ifdef WITH_TESTS
#include "harmonyosdevice_test.h"
#endif

#include <coreplugin/documentmanager.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorersettings.h>

#include <qtsupport/qtversionmanager.h>

#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

using namespace ProjectExplorer;

namespace HarmonyOs::Internal {

// Only the Qt Creator on the device has anywhere else to put them: its own home is in a
// sandbox that nothing else on the device can reach.
static void useUserStorageForProjects()
{
    if (!Utils::HostOsInfo::isHarmonyHost())
        return;
    const Utils::FilePath documents
        = Utils::FilePath::fromString(Constants::HARMONYOS_USER_DOCUMENTS);
    if (!documents.isDir())
        return;
    if (Core::DocumentManager::projectsDirectory() != Utils::PathChooser::homePath())
        return;
    Core::DocumentManager::setProjectsDirectory(documents);
    ProjectExplorer::globalProjectExplorerSettings().projectsDirectory.setValue(documents);
}

class HarmonyOsPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "HarmonyOS.json")

    void initialize() final
    {
        useUserStorageForProjects();
        settings(); // Load the SDK configuration before toolchains are detected.

        setupHarmonyOsSettingsPage();
        setupHarmonyOsDevice();
        setupHarmonyOsBuildDevice();
        setupHarmonyOsDeviceDetection();
        setupHarmonyOsQtVersion();
        setupHarmonyOsToolchain();

        setupHarmonyOsDeployConfiguration();
        setupHarmonyOsDeployStep();
        setupHarmonyOsRunSupport();
        setupHarmonyOsDebugSupport();

        connect(KitManager::instance(), &KitManager::kitsLoaded, this,
                &HarmonyOsPlugin::kitsRestored, Qt::SingleShotConnection);

#ifdef WITH_TESTS
        addTestCreator(createHarmonyOsDeviceTest);
        addTestCreator(createHarmonyOsManifestTest);
        addTestCreator(Sdk::createHarmonyOsSdkTest);
#endif
    }

    void kitsRestored()
    {
        applyConfig();
        connect(QtSupport::QtVersionManager::instance(),
                &QtSupport::QtVersionManager::qtVersionsChanged,
                this, [] { applyConfig(); });
    }
};

} // namespace HarmonyOs::Internal

#include "harmonyosplugin.moc"
