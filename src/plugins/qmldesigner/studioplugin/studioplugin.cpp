// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "studioplugin.h"

namespace QmlDesigner {

StudioPlugin::StudioPlugin()
{
}

QString StudioPlugin::pluginName() const
{
    return QLatin1String("StudioPlugin");
}

QString StudioPlugin::metaInfo() const
{
    return QLatin1String(":/studioplugin/studioplugin.metainfo");
}

}
