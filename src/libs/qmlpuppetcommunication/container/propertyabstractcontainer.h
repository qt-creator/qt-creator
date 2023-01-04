// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <qmetatype.h>
#include <QString>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class PropertyAbstractContainer;

QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container);
QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container);

class PropertyAbstractContainer
{

    friend QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container);
    friend QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container);
    friend QDebug operator <<(QDebug debug, const PropertyAbstractContainer &container);
public:
    PropertyAbstractContainer();
    PropertyAbstractContainer(qint32 instanceId, const PropertyName &name, const TypeName &dynamicTypeName);

    qint32 instanceId() const;
    PropertyName name() const;
    bool isDynamic() const;
    TypeName dynamicTypeName() const;

private:
    qint32 m_instanceId;
    PropertyName m_name;
    TypeName m_dynamicTypeName;
};

QDebug operator <<(QDebug debug, const PropertyAbstractContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyAbstractContainer)
