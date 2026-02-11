// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesigneraiplugin.h"

#include "aiassistantview.h"

#include <designersettings.h>
#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

namespace QmlDesigner {

static bool enableAiAssistant()
{
    return designerSettings().value(DesignerSettingsKey::GroqApiKey).toBool();
}

QmlDesignerAiPlugin::QmlDesignerAiPlugin() {}
QmlDesignerAiPlugin::~QmlDesignerAiPlugin() {}

Utils::Result<> QmlDesignerAiPlugin::initialize(const QStringList &arguments)
{
    return ExtensionSystem::IPlugin::initialize(arguments);
}

bool QmlDesignerAiPlugin::delayedInitialize()
{
    if (enableAiAssistant()) {
        QmlDesignerPlugin::instance()->viewManager().registerView(std::make_unique<AiAssistantView>(
            QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));
    }

    return true;
}

} // namespace QmlDesigner
