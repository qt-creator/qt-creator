// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <QVariant>
#include <QSharedPointer>

#include <memory>

namespace QmlDesigner {

namespace Internal {

class InternalBindingProperty;
class InternalSignalHandlerProperty;
class InternalSignalDeclarationProperty;
class InternalVariantProperty;
class InternalNodeListProperty;
class InternalNodeProperty;
class InternalNodeAbstractProperty;
class InternalNode;

using InternalNodePointer = std::shared_ptr<InternalNode>;

class QMLDESIGNERCORE_EXPORT InternalProperty
{
public:
    using Pointer = QSharedPointer<InternalProperty>;

    InternalProperty();
    virtual ~InternalProperty();

    virtual bool isValid() const;

    PropertyName name() const;

    virtual bool isBindingProperty() const;
    virtual bool isVariantProperty() const;
    virtual bool isNodeListProperty() const;
    virtual bool isNodeProperty() const;
    virtual bool isNodeAbstractProperty() const;
    virtual bool isSignalHandlerProperty() const;
    virtual bool isSignalDeclarationProperty() const;

    QSharedPointer<InternalBindingProperty> toBindingProperty() const;
    QSharedPointer<InternalVariantProperty> toVariantProperty() const;
    QSharedPointer<InternalNodeListProperty> toNodeListProperty() const;
    QSharedPointer<InternalNodeProperty> toNodeProperty() const;
    QSharedPointer<InternalNodeAbstractProperty> toNodeAbstractProperty() const;
    QSharedPointer<InternalSignalHandlerProperty> toSignalHandlerProperty() const;
    QSharedPointer<InternalSignalDeclarationProperty> toSignalDeclarationProperty() const;

    InternalNodePointer propertyOwner() const;

    virtual void remove();

    TypeName dynamicTypeName() const;

    void resetDynamicTypeName();

protected: // functions
    InternalProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);
    Pointer internalPointer() const;
    void setInternalWeakPointer(const Pointer &pointer);
    void setDynamicTypeName(const TypeName &name);
private:
    QWeakPointer<InternalProperty> m_internalPointer;
    PropertyName m_name;
    TypeName m_dynamicType;
    std::weak_ptr<InternalNode> m_propertyOwner;
};

} // namespace Internal
} // namespace QmlDesigner
