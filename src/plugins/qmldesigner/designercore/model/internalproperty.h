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

class QMLDESIGNERCORE_EXPORT InternalProperty : public std::enable_shared_from_this<InternalProperty>
{
public:
    using Pointer = std::shared_ptr<InternalProperty>;

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

    std::shared_ptr<InternalBindingProperty> toBindingProperty();
    std::shared_ptr<InternalVariantProperty> toVariantProperty();
    std::shared_ptr<InternalNodeListProperty> toNodeListProperty();
    std::shared_ptr<InternalNodeProperty> toNodeProperty();
    std::shared_ptr<InternalNodeAbstractProperty> toNodeAbstractProperty();
    std::shared_ptr<InternalSignalHandlerProperty> toSignalHandlerProperty();
    std::shared_ptr<InternalSignalDeclarationProperty> toSignalDeclarationProperty();

    InternalNodePointer propertyOwner() const;

    virtual void remove();

    TypeName dynamicTypeName() const;

    void resetDynamicTypeName();

protected: // functions
    InternalProperty(const PropertyName &name, const InternalNodePointer &propertyOwner);
    void setDynamicTypeName(const TypeName &name);
private:
    PropertyName m_name;
    TypeName m_dynamicType;
    std::weak_ptr<InternalNode> m_propertyOwner;
};

} // namespace Internal
} // namespace QmlDesigner
