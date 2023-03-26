// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentsplugin.h"

#include "tabviewindexmodel.h"
#include "addtabdesigneraction.h"
#include "entertabdesigneraction.h"

#include <designeractionmanager.h>
#include <viewmanager.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

ComponentsPlugin::ComponentsPlugin()
{
    TabViewIndexModel::registerDeclarativeType();
    DesignerActionManager *actionManager = &QmlDesignerPlugin::instance()->viewManager().designerActionManager();
    actionManager->addDesignerAction(new AddTabDesignerAction);
    actionManager->addDesignerAction(new EnterTabDesignerAction);
}

QString ComponentsPlugin::pluginName() const
{
    return QLatin1String("ComponentsPlugin");
}

QString ComponentsPlugin::metaInfo() const
{
    return QLatin1String(":/componentsplugin/components.metainfo");
}

}

