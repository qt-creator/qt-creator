// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerplugin.h"

#include "effectmakerview.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/externaltool.h>
#include <coreplugin/externaltoolmanager.h>

#include <componentcore_constants.h>
#include <designeractionmanager.h>
#include <viewmanager.h>
#include <qmldesigner/dynamiclicensecheck.h>
#include <qmldesignerplugin.h>
#include <modelnodeoperations.h>

#include <QJsonDocument>
#include <QMap>

namespace EffectMaker {

bool EffectMakerPlugin::delayedInitialize()
{
    if (m_delayedInitialized)
        return true;

    auto *designerPlugin = QmlDesigner::QmlDesignerPlugin::instance();
    auto &viewManager = designerPlugin->viewManager();
    viewManager.registerView(std::make_unique<EffectMakerView>(
        QmlDesigner::QmlDesignerPlugin::externalDependenciesForPluginInitializationOnly()));

    m_delayedInitialized = true;

    return true;
}

} // namespace EffectMaker

