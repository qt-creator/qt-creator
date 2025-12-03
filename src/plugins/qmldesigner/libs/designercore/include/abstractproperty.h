// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <designercore/tracing/qmldesignertracing.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace QmlDesigner {
    namespace Internal {
    class InternalNode;
    class InternalProperty;

    using InternalNodePointer = std::shared_ptr<InternalNode>;
    using InternalPropertyPointer = std::shared_ptr<InternalProperty>;
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


namespace Internal {
    class InternalNode;
    class ModelPrivate;
}

class QMLDESIGNERCORE_EXPORT AbstractProperty
{
    friend ModelNode;
    friend Internal::ModelPrivate;

    using SL = ModelTracing::SourceLocation;

public:
    AbstractProperty() = default;
    AbstractProperty(const AbstractProperty &) = default;
    AbstractProperty &operator=(const AbstractProperty &) = default;
    AbstractProperty(AbstractProperty &&) = default;
    AbstractProperty &operator=(AbstractProperty &&) noexcept = default;
    ~AbstractProperty();
    AbstractProperty(const AbstractProperty &property, AbstractView *view);
    AbstractProperty(PropertyNameView propertyName,
                     const Internal::InternalNodePointer &internalNode,
                     Model *model,
                     AbstractView *view);

    PropertyNameView name() const & { return m_propertyName; }

    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    bool exists() const;
    ModelNode parentModelNode() const;

    bool isDefaultProperty(SL sl = {}) const;
    VariantProperty toVariantProperty(SL sl = {}) const;
    NodeListProperty toNodeListProperty(SL sl = {}) const;
    NodeAbstractProperty toNodeAbstractProperty(SL sl = {}) const;
    BindingProperty toBindingProperty(SL sl = {}) const;
    NodeProperty toNodeProperty(SL sl = {}) const;
    SignalHandlerProperty toSignalHandlerProperty(SL sl = {}) const;
    SignalDeclarationProperty toSignalDeclarationProperty(SL sl = {}) const;

    bool isVariantProperty(SL sl = {}) const;
    bool isNodeListProperty(SL sl = {}) const;
    bool isNodeAbstractProperty(SL sl = {}) const;
    bool isBindingProperty(SL sl = {}) const;
    bool isNodeProperty(SL sl = {}) const;
    bool isSignalHandlerProperty(SL sl = {}) const;
    bool isSignalDeclarationProperty(SL sl = {}) const;

    PropertyType type(SL sl = {}) const;

    bool isDynamic(SL sl = {}) const;
    TypeName dynamicTypeName(SL sl = {}) const;

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

    friend size_t qHash(const AbstractProperty &property)
    {
        return ::qHash(property.m_internalNode.get()) ^ ::qHash(property.m_propertyName);
    }

    friend bool operator==(const AbstractProperty &first, const AbstractProperty &second)
    {
        return first.m_internalNode == second.m_internalNode
               && first.m_propertyName == second.m_propertyName;
    }

    friend bool operator!=(const AbstractProperty &first, const AbstractProperty &second)
    {
        return !(first == second);
    }

    friend bool operator<(const AbstractProperty &first, const AbstractProperty &second)
    {
        return std::tie(first.m_internalNode, first.m_propertyName)
               < std::tie(second.m_internalNode, second.m_propertyName);
    }

protected:
    AbstractProperty(const Internal::InternalPropertyPointer &property,
                     Model *model,
                     AbstractView *view);

    Internal::InternalNode *internalNode() const { return m_internalNode.get(); }

    Internal::InternalNodePointer internalNodeSharedPointer() const { return m_internalNode; }

    Internal::ModelPrivate *privateModel() const;

private:
    Utils::SmallString m_propertyName;
    Internal::InternalNodePointer m_internalNode;
    QPointer<Model> m_model;
    QPointer<AbstractView> m_view;
};

using AbstractProperties = QList<AbstractProperty>;

QMLDESIGNERCORE_EXPORT QTextStream& operator<<(QTextStream &stream, const AbstractProperty &property);
QMLDESIGNERCORE_EXPORT QDebug operator<<(QDebug debug, const AbstractProperty &AbstractProperty);
}
