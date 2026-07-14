// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyosconfigurations.h"
#include "harmonyosdevice.h"
#include "harmonyosqtversion.h"
#include "harmonyossettings.h"
#include "harmonyostoolchain.h"

#include <extensionsystem/iplugin.h>

#include <projectexplorer/kitmanager.h>

#include <qtsupport/qtversionmanager.h>

using namespace ProjectExplorer;

namespace HarmonyOs::Internal {

class HarmonyOsPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "HarmonyOS.json")

    void initialize() final
    {
        settings(); // Load the SDK configuration before toolchains are detected.

        setupHarmonyOsSettingsPage();
        setupHarmonyOsDevice();
        setupHarmonyOsQtVersion();
        setupHarmonyOsToolchain();

        connect(KitManager::instance(), &KitManager::kitsLoaded, this,
                &HarmonyOsPlugin::kitsRestored, Qt::SingleShotConnection);
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
