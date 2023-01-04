// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <QMetaType>
#include <QVariant>
#include <QString>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class PropertyValueContainer
{
public:
    PropertyValueContainer();
    PropertyValueContainer(qint32 instanceId,
                           const PropertyName &name,
                           const QVariant &value,
                           const TypeName &dynamicTypeName,
                           AuxiliaryDataType auxiliaryDataType = AuxiliaryDataType::None);
    PropertyValueContainer(qint32 option);

    qint32 instanceId() const;
    PropertyName name() const;
    QVariant value() const;
    bool isDynamic() const;
    TypeName dynamicTypeName() const;
    void setReflectionFlag(bool b);
    bool isReflected() const;
    AuxiliaryDataType auxiliaryDataType() const;

    friend QDataStream &operator<<(QDataStream &out, const PropertyValueContainer &container);
    friend QDataStream &operator>>(QDataStream &in, PropertyValueContainer &container);
    friend bool operator==(const PropertyValueContainer &first,
                           const PropertyValueContainer &second);
    friend bool operator<(const PropertyValueContainer &first, const PropertyValueContainer &second);

private:
    qint32 m_instanceId;
    PropertyName m_name;
    QVariant m_value;
    TypeName m_dynamicTypeName;
    AuxiliaryDataType m_auxiliaryDataType = AuxiliaryDataType::None;
    bool m_isReflected = false;
};

QDebug operator <<(QDebug debug, const PropertyValueContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyValueContainer)
