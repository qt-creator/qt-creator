// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <QString>
#include <QDataStream>
#include <QVariant>

namespace QmlDesigner {

class PropertyContainer;

QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer);
QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const PropertyContainer &propertyContainer);

class QMLDESIGNERCORE_EXPORT PropertyContainer
{
    friend QMLDESIGNERCORE_EXPORT QDataStream &operator<<(QDataStream &stream, const PropertyContainer &propertyContainer);
    friend QMLDESIGNERCORE_EXPORT QDataStream &operator>>(QDataStream &stream, PropertyContainer &propertyContainer);
    friend QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const PropertyContainer &propertyContainer);

public:
    PropertyContainer();
    PropertyContainer(const PropertyName &name, const QString &type, const QVariant &value);

    bool isValid() const;

    PropertyName name() const;
    QVariant value() const;
    QString type() const;


    void setValue(const QVariant &value);
    void set(const PropertyName &name, const QString &type, const QVariant &value);


private:
    PropertyName m_name;
    QString m_type;
    mutable QVariant m_value;
};

} //namespace QmlDesigner
