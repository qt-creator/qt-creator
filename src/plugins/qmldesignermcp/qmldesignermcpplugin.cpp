// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignermcpplugin.h"

#include "aiassistantconstants.h"
#include "aiassistantview.h"
#include "aiprovidersettings.h"

#include <qmldesignerplugin.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <designersettings.h>

namespace QmlDesigner {

QmlDesignerMcpPlugin::QmlDesignerMcpPlugin() {}
QmlDesignerMcpPlugin::~QmlDesignerMcpPlugin() {}

Utils::Result<> QmlDesignerMcpPlugin::initialize(const QStringList &arguments)
{
    using namespace Core;
    IOptionsPage::registerCategory(
        Constants::aiAssistantSettingsPageCategory,
        tr("AI Assistant/Mcp"),
        ":/AiAssistant/images/aiKitIcon.png");
    m_settings = std::make_unique<AiProviderSettings>();
    return ExtensionSystem::IPlugin::initialize(arguments);
}

bool QmlDesignerMcpPlugin::delayedInitialize()
{
    AiAssistantView *view = QmlDesignerPlugin::instance()->viewManager().registerView(
        std::make_unique<AiAssistantView>(
            QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));

    m_settings->setView(view);
    return true;
}

} // namespace QmlDesigner
