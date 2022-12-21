// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalnode_p.h"
#include "internalproperty.h"
#include "internalvariantproperty.h"
#include "internalnodeproperty.h"
#include "internalnodelistproperty.h"

#include <QDebug>

#include <algorithm>
#include <utility>

namespace QmlDesigner {
namespace Internal {

InternalNodeAbstractProperty::Pointer InternalNode::parentProperty() const
{
    return m_parentProperty;
}
void InternalNode::setParentProperty(const InternalNodeAbstractProperty::Pointer &parent)
{
    InternalNodeAbstractProperty::Pointer parentProperty = m_parentProperty.toStrongRef();
    if (parentProperty)
        parentProperty->remove(shared_from_this());

    Q_ASSERT(parent && parent->isValid());
    m_parentProperty = parent;

    parent->add(shared_from_this());
}

void InternalNode::resetParentProperty()
{
    InternalNodeAbstractProperty::Pointer parentProperty = m_parentProperty.toStrongRef();
    if (parentProperty)
        parentProperty->remove(shared_from_this());

    m_parentProperty.clear();
}

namespace {

template<typename Type>
auto find(Type &&auxiliaryDatas, AuxiliaryDataKeyView key)
{
    return std::find_if(auxiliaryDatas.begin(), auxiliaryDatas.end(), [&](const auto &element) {
        return element.first == key;
    });
}

} // namespace

std::optional<QVariant> InternalNode::auxiliaryData(AuxiliaryDataKeyView key) const
{
    auto found = find(m_auxiliaryDatas, key);

    if (found != m_auxiliaryDatas.end())
        return found->second;

    return {};
}

bool InternalNode::setAuxiliaryData(AuxiliaryDataKeyView key, const QVariant &data)
{
    auto found = find(m_auxiliaryDatas, key);

    if (found != m_auxiliaryDatas.end()) {
        if (found->second == data)
            return false;
        found->second = data;
    } else {
        m_auxiliaryDatas.emplace_back(AuxiliaryDataKey{key}, data);
    }

    return true;
}

bool InternalNode::removeAuxiliaryData(AuxiliaryDataKeyView key)
{
    auto found = find(m_auxiliaryDatas, key);

    if (found == m_auxiliaryDatas.end())
        return false;

    *found = std::move(m_auxiliaryDatas.back());

    m_auxiliaryDatas.pop_back();

    return true;
}

bool InternalNode::hasAuxiliaryData(AuxiliaryDataKeyView key) const
{
    auto found = find(m_auxiliaryDatas, key);

    return found != m_auxiliaryDatas.end();
}

AuxiliaryDatasForType InternalNode::auxiliaryData(AuxiliaryDataType type) const
{
    AuxiliaryDatasForType data;
    data.reserve(m_auxiliaryDatas.size());

    for (const auto &element : m_auxiliaryDatas) {
        if (element.first.type == type)
            data.emplace_back(element.first.name, element.second);
    }

    return data;
}

InternalProperty::Pointer InternalNode::property(const PropertyName &name) const
{
    return m_namePropertyHash.value(name);
}

InternalBindingProperty::Pointer InternalNode::bindingProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isBindingProperty())
        return property.staticCast<InternalBindingProperty>();

    return InternalBindingProperty::Pointer();
}

InternalSignalHandlerProperty::Pointer InternalNode::signalHandlerProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isSignalHandlerProperty())
        return property.staticCast<InternalSignalHandlerProperty>();

    return InternalSignalHandlerProperty::Pointer();
}

InternalSignalDeclarationProperty::Pointer InternalNode::signalDeclarationProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isSignalDeclarationProperty())
        return property.staticCast<InternalSignalDeclarationProperty>();

    return InternalSignalDeclarationProperty::Pointer();
}

InternalVariantProperty::Pointer InternalNode::variantProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isVariantProperty())
        return property.staticCast<InternalVariantProperty>();

    return InternalVariantProperty::Pointer();
}

void InternalNode::addBindingProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalBindingProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addSignalHandlerProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(
        InternalSignalHandlerProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addSignalDeclarationProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(
        InternalSignalDeclarationProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

InternalNodeListProperty::Pointer InternalNode::nodeListProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property && property->isNodeListProperty())
        return property.staticCast<InternalNodeListProperty>();

    return InternalNodeListProperty::Pointer();
}

InternalNodeAbstractProperty::Pointer InternalNode::nodeAbstractProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property && property->isNodeAbstractProperty())
        return property.staticCast<InternalNodeAbstractProperty>();

    return InternalNodeProperty::Pointer();
}

InternalNodeProperty::Pointer InternalNode::nodeProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isNodeProperty())
        return property.staticCast<InternalNodeProperty>();

    return InternalNodeProperty::Pointer();
}

void InternalNode::addVariantProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalVariantProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addNodeProperty(const PropertyName &name, const TypeName &dynamicTypeName)
{
    InternalNodeProperty::Pointer newProperty(InternalNodeProperty::create(name, shared_from_this()));
    newProperty->setDynamicTypeName(dynamicTypeName);
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addNodeListProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalNodeListProperty::create(name, shared_from_this()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::removeProperty(const PropertyName &name)
{
    InternalProperty::Pointer property = m_namePropertyHash.take(name);
    Q_ASSERT(!property.isNull());
}

PropertyNameList InternalNode::propertyNameList() const
{
    return m_namePropertyHash.keys();
}

bool InternalNode::hasProperties() const
{
    return !m_namePropertyHash.isEmpty();
}

bool InternalNode::hasProperty(const PropertyName &name) const
{
    return m_namePropertyHash.contains(name);
}

QList<InternalProperty::Pointer> InternalNode::propertyList() const
{
    return m_namePropertyHash.values();
}

QList<InternalNodeAbstractProperty::Pointer> InternalNode::nodeAbstractPropertyList() const
{
    QList<InternalNodeAbstractProperty::Pointer> abstractPropertyList;
    const QList<InternalProperty::Pointer> properties = propertyList();
    for (const InternalProperty::Pointer &property : properties) {
        if (property->isNodeAbstractProperty())
            abstractPropertyList.append(property->toNodeAbstractProperty());
    }

    return abstractPropertyList;
}


QList<InternalNode::Pointer> InternalNode::allSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;
    const QList<InternalNodeAbstractProperty::Pointer> properties = nodeAbstractPropertyList();
    for (const InternalNodeAbstractProperty::Pointer &property : properties) {
        nodeList.append(property->allSubNodes());
    }

    return nodeList;
}

QList<InternalNode::Pointer> InternalNode::allDirectSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;
    const QList<InternalNodeAbstractProperty::Pointer> properties = nodeAbstractPropertyList();
    for (const InternalNodeAbstractProperty::Pointer &property : properties) {
        nodeList.append(property->directSubNodes());
    }

    return nodeList;
}

} // namespace Internal
}
