// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "insightplugin.h"

#include "insightview.h"

#include <qmldesigner/dynamiclicensecheck.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

InsightPlugin::InsightPlugin() = default;
InsightPlugin::~InsightPlugin() = default;

bool InsightPlugin::initialize(const QStringList & /*arguments*/, QString * /*errorMessage*/)
{
    return true;
}

bool InsightPlugin::delayedInitialize()
{
    auto *designerPlugin = QmlDesignerPlugin::instance();
    auto &viewManager = designerPlugin->viewManager();
    viewManager.registerView(std::make_unique<InsightView>(
        QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));

    return true;
}

} // namespace QmlDesigner
