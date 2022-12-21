// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtquickplugin.h"

namespace QmlDesigner {

QtQuickPlugin::QtQuickPlugin() = default;

QString QtQuickPlugin::pluginName() const
{
    return QLatin1String("QtQuickPlugin");
}

QString QtQuickPlugin::metaInfo() const
{
    return QLatin1String(":/qtquickplugin/quick.metainfo");
}

}

