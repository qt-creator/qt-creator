// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQmlPropertyMap>
#include <QtQml>
#include "propertyeditorvalue.h"

namespace QmlDesigner {

class DesignerPropertyMap : public QQmlPropertyMap
{

public:
    DesignerPropertyMap(QObject *parent = nullptr);

    QVariant value(const QString &key) const;

    static void registerDeclarativeType(const QString &name);
};

} //QmlDesigner
