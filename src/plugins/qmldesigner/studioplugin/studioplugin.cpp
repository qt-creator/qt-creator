/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise QML Live Preview Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

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
