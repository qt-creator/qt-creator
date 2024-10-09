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
    return Core::ICore::isQtDesignStudio()
           && QmlDesignerPlugin::instance()->settings().value(DesignerSettingsKey::GROQ_API_KEY).toBool();
}

QmlDesignerAiPlugin::QmlDesignerAiPlugin() {}
QmlDesignerAiPlugin::~QmlDesignerAiPlugin() {}

bool QmlDesignerAiPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    return ExtensionSystem::IPlugin::initialize(arguments, errorString);
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
