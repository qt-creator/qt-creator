// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designerpropertymap.h"

#include "propertyeditortracing.h"

namespace QmlDesigner {

using PropertyEditorTracing::category;

DesignerPropertyMap::DesignerPropertyMap(QObject *parent) : QQmlPropertyMap(parent)
{
    NanotraceHR::Tracer tracer{"designer property map", category()};
}

QVariant DesignerPropertyMap::value(const QString &key) const
{
    NanotraceHR::Tracer tracer{"designer property map value", QmlDesigner::category()};

    if (contains(key))
        return QQmlPropertyMap::value(key);
    return QVariant();
}

void DesignerPropertyMap::registerDeclarativeType(const QString &name)
{
    NanotraceHR::Tracer tracer{"designer property map register declarative type",
                               QmlDesigner::category()};

    qmlRegisterType<DesignerPropertyMap>("Bauhaus", 1, 0, name.toUtf8());
}

} //QmlDesigner

