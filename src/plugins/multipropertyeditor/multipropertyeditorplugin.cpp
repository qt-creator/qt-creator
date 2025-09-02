// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "multipropertyeditorplugin.h"

#include "multipropertyeditoraction.h"
#include "multipropertyeditorview.h"

#include <propertyeditorview.h>

#include <coreplugin/icore.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

bool MultiPropertyEditorPlugin::delayedInitialize()
{
    auto *designerPlugin = QmlDesigner::QmlDesignerPlugin::instance();
    auto &viewManager = designerPlugin->viewManager();
    auto view = viewManager.registerView(std::make_unique<QmlDesigner::MultiPropertyEditorView>(
        designerPlugin->imageCache(),
        QmlDesigner::QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));

    auto standardPropertyEditorView = viewManager.propertyEditorView();
    view->unifiedAction()->registerView(standardPropertyEditorView);

    return true;
}

} // namespace QmlDesigner
