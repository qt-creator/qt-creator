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

#include <utils/pathchooser.h>

using namespace ProjectExplorer;

namespace HarmonyOs::Internal {

#ifdef Q_OS_OHOS
static void useUserStorageForProjects()
{
    const Utils::FilePath documents
        = Utils::FilePath::fromString(Constants::HARMONYOS_USER_DOCUMENTS);
    if (!documents.isDir())
        return;
    if (Core::DocumentManager::projectsDirectory() != Utils::PathChooser::homePath())
        return;
    Core::DocumentManager::setProjectsDirectory(documents);
    ProjectExplorer::globalProjectExplorerSettings().projectsDirectory.setValue(documents);
}
#endif

class HarmonyOsPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "HarmonyOS.json")

    void initialize() final
    {
#ifdef Q_OS_OHOS
        useUserStorageForProjects();
#endif
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
