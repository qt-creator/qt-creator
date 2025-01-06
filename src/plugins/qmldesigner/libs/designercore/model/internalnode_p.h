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
#include <tracing/qmldesignertracing.h>
#include <utils/smallstring.h>

#include <QHash>
#include <QMap>
#include <QSharedPointer>
#include <QStringList>
#include <QWeakPointer>

#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace QmlDesigner {

namespace Internal {
using namespace NanotraceHR::Literals;

class InternalProperty;
class InternalNode;

using InternalNodePointer = std::shared_ptr<InternalNode>;
using InternalPropertyPointer = std::shared_ptr<InternalProperty>;

using NanotraceHR::dictonary;
using NanotraceHR::keyValue;
using namespace std::literals::string_view_literals;

class InternalNode : public std::enable_shared_from_this<InternalNode>
{
    friend InternalProperty;

public:
    using Pointer = std::shared_ptr<InternalNode>;
    using WeakPointer = std::weak_ptr<InternalNode>;
    using FewNodes = QVarLengthArray<Pointer, 32>;
    using ManyNodes = QVarLengthArray<Pointer, 1024>;

    explicit InternalNode(TypeNameView typeName,
                          int majorVersion,
                          int minorVersion,
                          qint32 internalId,
                          ModelTracing::Category::FlowTokenType flowTraceToken)
        : typeName(typeName.toByteArray())
        , majorVersion(majorVersion)
        , minorVersion(minorVersion)
        , isValid(true)
        , internalId(internalId)
        , traceToken(flowTraceToken.beginAsynchronous("InternalNode",
                                                      keyValue("type", typeName),
                                                      keyValue("internal id", internalId)))
    {}

    InternalNodeAbstractProperty::Pointer parentProperty() const { return m_parentProperty.lock(); }

    // Reparent within model
    void setParentProperty(const InternalNodeAbstractProperty::Pointer &parent);
    void resetParentProperty();

    std::optional<QVariant> auxiliaryData(AuxiliaryDataKeyView key) const;
    bool setAuxiliaryData(AuxiliaryDataKeyView key, const QVariant &data);
    bool removeAuxiliaryData(AuxiliaryDataKeyView key);
    bool hasAuxiliaryData(AuxiliaryDataKeyView key) const;
    bool hasAuxiliaryData(AuxiliaryDataType type) const;
    AuxiliaryDatasForType auxiliaryData(AuxiliaryDataType type) const;
    AuxiliaryDatasView auxiliaryData() const { return std::as_const(m_auxiliaryDatas); }

    template<typename Type>
    Type *property(PropertyNameView name) const
    {
        auto propertyIter = m_nameProperties.find(name);

        if (propertyIter != m_nameProperties.end()) {
            if (auto property = propertyIter->second.get();
                property && property->propertyType() == Type::type)
                return static_cast<Type *>(property);
        }

        return {};
    }

    InternalProperty *property(PropertyNameView name) const
    {
        auto propertyIter = m_nameProperties.find(name);
        if (propertyIter != m_nameProperties.end())
            return propertyIter->second.get();

        return nullptr;
    }

    auto bindingProperty(PropertyNameView name) const
    {
        return property<InternalBindingProperty>(name);
    }

    auto signalHandlerProperty(PropertyNameView name) const
    {
        return property<InternalSignalHandlerProperty>(name);
    }

    auto signalDeclarationProperty(PropertyNameView name) const
    {
        return property<InternalSignalDeclarationProperty>(name);
    }

    auto variantProperty(PropertyNameView name) const
    {
        return property<InternalVariantProperty>(name);
    }

    auto nodeListProperty(PropertyNameView name) const
    {
        return property<InternalNodeListProperty>(name);
    }

    InternalNodeAbstractProperty::Pointer nodeAbstractProperty(PropertyNameView name) const
    {
        auto found = m_nameProperties.find(name);
        if (found != m_nameProperties.end()) {
            auto property = found->second;
            auto propertyType = property->propertyType();
            if (propertyType == PropertyType::NodeList || propertyType == PropertyType::Node) {
                return std::static_pointer_cast<InternalNodeAbstractProperty>(property);
            }
        }
        return {};
    }

    auto nodeProperty(PropertyNameView name) const { return property<InternalNodeProperty>(name); }

    template<typename Type>
    Type *addProperty(PropertyNameView name)
    {
        auto [iter, inserted] = m_nameProperties.try_emplace(
            name, std::make_shared<Type>(name, shared_from_this()));

        if (inserted)
            return static_cast<Type *>(iter->second.get());

        return nullptr;
    }

    auto addBindingProperty(PropertyNameView name)
    {
        return addProperty<InternalBindingProperty>(name);
    }

    auto addSignalHandlerProperty(PropertyNameView name)
    {
        return addProperty<InternalSignalHandlerProperty>(name);
    }

    auto addSignalDeclarationProperty(PropertyNameView name)
    {
        return addProperty<InternalSignalDeclarationProperty>(name);
    }

    auto addNodeListProperty(PropertyNameView name)
    {
        return addProperty<InternalNodeListProperty>(name);
    }

    auto addVariantProperty(PropertyNameView name)
    {
        return addProperty<InternalVariantProperty>(name);
    }

    auto addNodeProperty(PropertyNameView name, const TypeName &dynamicTypeName)
    {
        auto property = addProperty<InternalNodeProperty>(name);
        property->setDynamicTypeName(dynamicTypeName);

        return property;
    }

    PropertyNameList propertyNameList() const;
    PropertyNameViews propertyNameViews() const;

    bool hasProperties() const { return m_nameProperties.size(); }

    ManyNodes allSubNodes() const;
    ManyNodes allDirectSubNodes() const;
    void addSubNodes(ManyNodes &nodes) const;
    void addDirectSubNodes(ManyNodes &nodes) const;
    static void addSubNodes(ManyNodes &nodes, const InternalProperty *property);
    static void addDirectSubNodes(ManyNodes &nodes, const InternalProperty *property);

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

    void removeProperty(PropertyNameView name)
    {
        auto found = m_nameProperties.find(name);
        m_nameProperties.erase(found); // C++ 23 -> m_nameProperties.erase(name)
    }

    using PropertyDict = std::map<Utils::SmallString, InternalPropertyPointer, std::less<>>;

    PropertyDict::const_iterator begin() const { return m_nameProperties.begin(); }

    PropertyDict::const_iterator end() const { return m_nameProperties.end(); }

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
    NO_UNIQUE_ADDRESS ModelTracing::AsynchronousToken traceToken;

private:
    AuxiliaryDatas m_auxiliaryDatas;
    InternalNodeAbstractProperty::WeakPointer m_parentProperty;
    PropertyDict m_nameProperties;
};

} // Internal
} // QtQmlDesigner
