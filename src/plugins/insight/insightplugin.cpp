// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "insightview.h"

#include <extensionsystem/iplugin.h>

#include <qmldesigner/dynamiclicensecheck.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

class InsightPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Insight.json")

    bool delayedInitialize() final
    {
        auto *designerPlugin = QmlDesignerPlugin::instance();
        auto &viewManager = designerPlugin->viewManager();
        viewManager.registerView(std::make_unique<InsightView>(
            QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));

        return true;
    }
};

} // QmlDesigner

#include "insightplugin.moc"
