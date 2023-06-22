// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentsplugin.h"

#include <qmldesignerplugin.h>

namespace QmlDesigner {

ComponentsPlugin::ComponentsPlugin()
{
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

