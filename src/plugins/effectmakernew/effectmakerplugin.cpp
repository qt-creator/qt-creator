// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerview.h"

#include <coreplugin/icore.h>
#include <extensionsystem/iplugin.h>

#include <viewmanager.h>
#include <qmldesignerplugin.h>

namespace EffectMaker {

static bool enableEffectMaker()
{
    Utils::QtcSettings *settings = Core::ICore::settings();
    const Utils::Key enableModelManagerKey = "QML/Designer/UseExperimentalFeatures44";

    return settings->value(enableModelManagerKey, false).toBool();
}

class EffectMakerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "EffectMakerNew.json")

    bool delayedInitialize() final
    {
        if (enableEffectMaker()) {
            auto *designerPlugin = QmlDesigner::QmlDesignerPlugin::instance();
            auto &viewManager = designerPlugin->viewManager();

            viewManager.registerView(std::make_unique<EffectMakerView>(
                QmlDesigner::QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));
        }
        return true;
    }
};

} // namespace EffectMaker

#include "effectmakerplugin.moc"
