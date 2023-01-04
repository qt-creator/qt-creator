// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <qmetatype.h>
#include <QString>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class PropertyBindingContainer
{
    friend QDataStream &operator>>(QDataStream &in, PropertyBindingContainer &container);

public:
    PropertyBindingContainer();
    PropertyBindingContainer(qint32 instanceId, const PropertyName &name, const QString &expression, const TypeName &dynamicTypeName);

    qint32 instanceId() const;
    PropertyName name() const;
    QString expression() const;
    bool isDynamic() const;
    TypeName dynamicTypeName() const;

private:
    qint32 m_instanceId;
    PropertyName m_name;
    QString m_expression;
    TypeName m_dynamicTypeName;
};

QDataStream &operator<<(QDataStream &out, const PropertyBindingContainer &container);
QDataStream &operator>>(QDataStream &in, PropertyBindingContainer &container);

QDebug operator <<(QDebug debug, const PropertyBindingContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyBindingContainer)
