// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmldesignerliteplugin.h"

#include <qmldesignerbase/qmldesignerbaseplugin.h>

namespace QmlDesigner {

QmlDesignerLitePlugin::QmlDesignerLitePlugin()
{
    QmlDesignerBasePlugin::enbableLiteMode();
}

QmlDesignerLitePlugin::~QmlDesignerLitePlugin() = default;

bool QmlDesignerLitePlugin::initialize(const QStringList &, QString *)
{
    return true;
}

} // namespace QmlDesigner
