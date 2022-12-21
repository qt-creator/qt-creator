// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designerpropertymap.h"

namespace QmlDesigner {

DesignerPropertyMap::DesignerPropertyMap(QObject *parent) : QQmlPropertyMap(parent)
{
}

QVariant DesignerPropertyMap::value(const QString &key) const
{
    if (contains(key))
        return QQmlPropertyMap::value(key);
    return QVariant();
}

void DesignerPropertyMap::registerDeclarativeType(const QString &name)
{
    qmlRegisterType<DesignerPropertyMap>("Bauhaus",1,0,name.toUtf8());
}

} //QmlDesigner

