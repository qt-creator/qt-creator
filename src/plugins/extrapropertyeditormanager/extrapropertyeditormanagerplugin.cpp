// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extrapropertyeditormanagerplugin.h"
#include "extrapropertyeditorview.h"

#include <coreplugin/icore.h>
#include <qmldesignerplugin.h>

namespace ExtraPropertyEditorManager {

bool ExtraPropertyEditorManagerPlugin::delayedInitialize()
{
    auto *designerPlugin = QmlDesigner::QmlDesignerPlugin::instance();
    auto &viewManager = designerPlugin->viewManager();
    viewManager.registerView(std::make_unique<QmlDesigner::ExtraPropertyEditorView>(
        designerPlugin->imageCache(),
        QmlDesigner::QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));

    return true;
}

} //  namespace ExtraPropertyEditorManager
