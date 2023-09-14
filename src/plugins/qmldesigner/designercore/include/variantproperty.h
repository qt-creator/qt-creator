// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"
#include "abstractproperty.h"

#include <enumeration.h>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractView;

namespace Internal { class ModelPrivate; }

class QMLDESIGNERCORE_EXPORT VariantProperty final : public AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;
    friend AbstractProperty;

public:
    void setValue(const QVariant &value);
    QVariant value() const;

    void setEnumeration(const EnumerationName &enumerationName);
    Enumeration enumeration() const;
    bool holdsEnumeration() const;

    void setDynamicTypeNameAndValue(const TypeName &type, const QVariant &value);
    void setDynamicTypeNameAndEnumeration(const TypeName &type, const EnumerationName &enumerationName);

    VariantProperty();
    VariantProperty(const VariantProperty &property, AbstractView *view);
    VariantProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
};

QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const VariantProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const VariantProperty &VariantProperty);

}
