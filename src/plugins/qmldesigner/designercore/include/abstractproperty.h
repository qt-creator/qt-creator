// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPointer>
#include <QSharedPointer>
#include "qmldesignercorelib_global.h"

#include <memory>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {
    namespace Internal {
    class InternalNode;
    class InternalProperty;

    using InternalNodePointer = std::shared_ptr<InternalNode>;
    using InternalPropertyPointer = QSharedPointer<InternalProperty>;
    }

class Model;
class ModelNode;
class AbstractView;
class QMLDESIGNERCORE_EXPORT VariantProperty;
class QMLDESIGNERCORE_EXPORT NodeListProperty;
class QMLDESIGNERCORE_EXPORT NodeAbstractProperty;
class QMLDESIGNERCORE_EXPORT BindingProperty;
class QMLDESIGNERCORE_EXPORT NodeProperty;
class QMLDESIGNERCORE_EXPORT SignalHandlerProperty;
class QMLDESIGNERCORE_EXPORT SignalDeclarationProperty;
class QmlObjectNode;


namespace Internal {
    class InternalNode;
    class ModelPrivate;
}

class QMLDESIGNERCORE_EXPORT AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;

    friend QMLDESIGNERCORE_EXPORT bool operator ==(const AbstractProperty &property1, const AbstractProperty &property2);
    friend QMLDESIGNERCORE_EXPORT bool operator !=(const AbstractProperty &property1, const AbstractProperty &property2);

public:
    AbstractProperty() = default;
    ~AbstractProperty();
    AbstractProperty(const AbstractProperty &property, AbstractView *view);

    PropertyName name() const;

    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    bool exists() const;
    ModelNode parentModelNode() const;
    QmlObjectNode parentQmlObjectNode() const;

    bool isDefaultProperty() const;
    VariantProperty toVariantProperty() const;
    NodeListProperty toNodeListProperty() const;
    NodeAbstractProperty toNodeAbstractProperty() const;
    BindingProperty toBindingProperty() const;
    NodeProperty toNodeProperty() const;
    SignalHandlerProperty toSignalHandlerProperty() const;
    SignalDeclarationProperty toSignalDeclarationProperty() const;

    bool isVariantProperty() const;
    bool isNodeListProperty() const;
    bool isNodeAbstractProperty() const;
    bool isBindingProperty() const;
    bool isNodeProperty() const;
    bool isSignalHandlerProperty() const;
    bool isSignalDeclarationProperty() const;

    bool isDynamic() const;
    TypeName dynamicTypeName() const;

    template<typename... TypeName>
    bool hasDynamicTypeName(const TypeName &...typeName) const
    {
        auto dynamicTypeName_ = dynamicTypeName();
        return ((dynamicTypeName_ == typeName) || ...);
    }

    template<typename... TypeName>
    bool hasDynamicTypeName(const std::tuple<TypeName...> &typeNames) const
    {
        return std::apply([&](auto... typeName) { return hasDynamicTypeName(typeName...); },
                          typeNames);
    }

    Model *model() const;
    AbstractView *view() const;

    friend auto qHash(const AbstractProperty &property)
    {
        return ::qHash(property.m_internalNode.get()) ^ ::qHash(property.m_propertyName);
    }

protected:
    AbstractProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view);
    AbstractProperty(const Internal::InternalPropertyPointer &property, Model* model, AbstractView *view);
    Internal::InternalNodePointer internalNode() const;
    Internal::ModelPrivate *privateModel() const;

private:
    PropertyName m_propertyName;
    Internal::InternalNodePointer m_internalNode;
    QPointer<Model> m_model;
    QPointer<AbstractView> m_view;
};

QMLDESIGNERCORE_EXPORT bool operator ==(const AbstractProperty &property1, const AbstractProperty &property2);
QMLDESIGNERCORE_EXPORT bool operator !=(const AbstractProperty &property1, const AbstractProperty &property2);
QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const AbstractProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const AbstractProperty &AbstractProperty);
}
