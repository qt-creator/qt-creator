// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "internalbindingproperty.h"
#include "internalnodeabstractproperty.h"
#include "internalnodelistproperty.h"
#include "internalnodeproperty.h"
#include "internalproperty.h"
#include "internalsignalhandlerproperty.h"
#include "internalvariantproperty.h"

#include <auxiliarydata.h>
#include <projectstorageids.h>
#include <utils/smallstring.h>

#include <QHash>
#include <QMap>
#include <QSharedPointer>
#include <QStringList>
#include <QWeakPointer>

#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace QmlDesigner {

namespace Internal {

class InternalProperty;
class InternalNode;

using InternalNodePointer = std::shared_ptr<InternalNode>;
using InternalPropertyPointer = std::shared_ptr<InternalProperty>;

class InternalNode : public std::enable_shared_from_this<InternalNode>
{
    friend InternalProperty;

public:
    using Pointer = std::shared_ptr<InternalNode>;
    using WeakPointer = std::weak_ptr<InternalNode>;

    explicit InternalNode(TypeName typeName, int majorVersion, int minorVersion, qint32 internalId)
        : typeName(std::move(typeName))
        , majorVersion(majorVersion)
        , minorVersion(minorVersion)
        , isValid(true)
        , internalId(internalId)
    {}

    InternalNodeAbstractProperty::Pointer parentProperty() const;

    // Reparent within model
    void setParentProperty(const InternalNodeAbstractProperty::Pointer &parent);
    void resetParentProperty();

    std::optional<QVariant> auxiliaryData(AuxiliaryDataKeyView key) const;
    bool setAuxiliaryData(AuxiliaryDataKeyView key, const QVariant &data);
    bool removeAuxiliaryData(AuxiliaryDataKeyView key);
    bool hasAuxiliaryData(AuxiliaryDataKeyView key) const;
    AuxiliaryDatasForType auxiliaryData(AuxiliaryDataType type) const;
    AuxiliaryDatasView auxiliaryData() const { return std::as_const(m_auxiliaryDatas); }

    template<typename Type>
    typename Type::Pointer property(const PropertyName &name) const
    {
        auto property = m_namePropertyHash.value(name);
        if (property && property->propertyType() == Type::type)
            return std::static_pointer_cast<Type>(property);

        return {};
    }

    InternalProperty::Pointer property(const PropertyName &name) const
    {
        return m_namePropertyHash.value(name);
    }

    auto bindingProperty(const PropertyName &name) const
    {
        return property<InternalBindingProperty>(name);
    }

    auto signalHandlerProperty(const PropertyName &name) const
    {
        return property<InternalSignalHandlerProperty>(name);
    }

    auto signalDeclarationProperty(const PropertyName &name) const
    {
        return property<InternalSignalDeclarationProperty>(name);
    }

    auto variantProperty(const PropertyName &name) const
    {
        return property<InternalVariantProperty>(name);
    }

    auto nodeListProperty(const PropertyName &name) const
    {
        return property<InternalNodeListProperty>(name);
    }

    InternalNodeAbstractProperty::Pointer nodeAbstractProperty(const PropertyName &name) const
    {
        auto property = m_namePropertyHash.value(name);
        if (property
            && (property->propertyType() == PropertyType::NodeList
                || property->propertyType() == PropertyType::Node)) {
            return std::static_pointer_cast<InternalNodeAbstractProperty>(property);
        }
        return {};
    }

    InternalNodeProperty::Pointer nodeProperty(const PropertyName &name) const
    {
        return property<InternalNodeProperty>(name);
    }

    template<typename Type>
    auto &addProperty(const PropertyName &name)
    {
        auto newProperty = std::make_shared<Type>(name, shared_from_this());
        auto inserted = m_namePropertyHash.insert(name, std::move(newProperty));

        return *inserted->get();
    }

    void addBindingProperty(const PropertyName &name)
    {
        addProperty<InternalBindingProperty>(name);
    }

    void addSignalHandlerProperty(const PropertyName &name)
    {
        addProperty<InternalSignalHandlerProperty>(name);
    }

    void addSignalDeclarationProperty(const PropertyName &name)
    {
        addProperty<InternalSignalDeclarationProperty>(name);
    }

    void addNodeListProperty(const PropertyName &name)
    {
        addProperty<InternalNodeListProperty>(name);
    }

    void addVariantProperty(const PropertyName &name)
    {
        addProperty<InternalVariantProperty>(name);
    }

    void addNodeProperty(const PropertyName &name, const TypeName &dynamicTypeName)
    {
        auto &property = addProperty<InternalNodeProperty>(name);
        property.setDynamicTypeName(dynamicTypeName);
    }

    PropertyNameList propertyNameList() const;

    bool hasProperties() const;
    bool hasProperty(const PropertyName &name) const;

    QList<InternalProperty::Pointer> propertyList() const;
    QList<InternalNodeAbstractProperty::Pointer> nodeAbstractPropertyList() const;
    QList<InternalNode::Pointer> allSubNodes() const;
    QList<InternalNode::Pointer> allDirectSubNodes() const;

    friend bool operator<(const InternalNode::Pointer &firstNode,
                          const InternalNode::Pointer &secondNode)
    {
        if (!firstNode)
            return true;

        if (!secondNode)
            return false;

        return firstNode->internalId < secondNode->internalId;
    }

    friend size_t qHash(const InternalNodePointer &node) { return ::qHash(node.get()); }

protected:
    void removeProperty(const PropertyName &name);

public:
    TypeName typeName;
    QString id;
    int majorVersion = 0;
    int minorVersion = 0;
    bool isValid = false;
    qint32 internalId = -1;
    QString nodeSource;
    int nodeSourceType = 0;
    QString behaviorPropertyName;
    QStringList scriptFunctions;
    ModuleId moduleId;
    ImportedTypeNameId importedTypeNameId;
    TypeId typeId;

private:
    AuxiliaryDatas m_auxiliaryDatas;
    InternalNodeAbstractProperty::WeakPointer m_parentProperty;
    QHash<PropertyName, InternalPropertyPointer> m_namePropertyHash;
};

} // Internal
} // QtQmlDesigner
