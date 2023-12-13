// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerview.h"

#include <extensionsystem/iplugin.h>

#include <viewmanager.h>
#include <qmldesignerplugin.h>

namespace EffectMaker {

class EffectMakerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "EffectMakerNew.json")

    bool delayedInitialize() final
    {
        auto designerPlugin = QmlDesigner::QmlDesignerPlugin::instance();
        auto &viewManager = designerPlugin->viewManager();
        viewManager.registerView(std::make_unique<EffectMakerView>(
            QmlDesigner::QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));

        return true;
    }
};

} // namespace EffectMaker

#include "effectmakerplugin.moc"
