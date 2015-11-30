/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "internalnode_p.h"
#include "internalproperty.h"
#include "internalvariantproperty.h"
#include "internalnodeproperty.h"
#include "internalnodelistproperty.h"


#include <QDebug>

namespace QmlDesigner {
namespace Internal {

/*!
  \class QmlDesigner::Internal::InternalNode

  Represents one XML element.
  */

InternalNode::InternalNode() :
    m_majorVersion(0),
    m_minorVersion(0),
    m_valid(false),
    m_internalId(-1)
{
}

InternalNode::InternalNode(const TypeName &typeName,int majorVersion, int minorVersion, qint32 internalId):
        m_typeName(typeName),
        m_majorVersion(majorVersion),
        m_minorVersion(minorVersion),
        m_valid(true),
        m_internalId(internalId)
{

}

InternalNode::Pointer InternalNode::create(const TypeName &type,int majorVersion, int minorVersion, qint32 internalId)
{
    InternalNode *newPointer(new InternalNode(type, majorVersion, minorVersion, internalId));
    InternalNode::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer);

    return smartPointer;
}

InternalNode::Pointer InternalNode::internalPointer() const
{
    return m_internalPointer.toStrongRef();
}
void InternalNode::setInternalWeakPointer(const Pointer &pointer)
{
    m_internalPointer = pointer;
}

TypeName InternalNode::type() const
{
    return m_typeName;
}

void InternalNode::setType(const TypeName &newType)
{
    m_typeName = newType;
}

int InternalNode::minorVersion() const
{
    return m_minorVersion;
}

int InternalNode::majorVersion() const
{
    return m_majorVersion;
}

void InternalNode::setMinorVersion(int number)
{
    m_minorVersion = number;
}

void InternalNode::setMajorVersion(int number)
{
    m_majorVersion = number;
}

bool InternalNode::isValid() const
{
    return m_valid;
}

void InternalNode::setValid(bool valid)
{
    m_valid = valid;
}

InternalNodeAbstractProperty::Pointer InternalNode::parentProperty() const
{
    return m_parentProperty;
}
void InternalNode::setParentProperty(const InternalNodeAbstractProperty::Pointer &parent)
{
    InternalNodeAbstractProperty::Pointer parentProperty = m_parentProperty.toStrongRef();
    if (parentProperty)
        parentProperty->remove(internalPointer());

    Q_ASSERT(parent && parent->isValid());
    m_parentProperty = parent;

    parent->add(internalPointer());
}

void InternalNode::resetParentProperty()
{
    InternalNodeAbstractProperty::Pointer parentProperty = m_parentProperty.toStrongRef();
    if (parentProperty)
        parentProperty->remove(internalPointer());

    m_parentProperty.clear();
}

QString InternalNode::id() const
{
    return m_id;
}

void InternalNode::setId(const QString& id)
{
    m_id = id;
}

bool InternalNode::hasId() const
{
    return !m_id.isEmpty();
}


uint qHash(const InternalNodePointer& node)
{
    if (node.isNull())
        return ::qHash(-1);

    return ::qHash(node->internalId());
}

QVariant InternalNode::auxiliaryData(const PropertyName &name) const
{
    return m_auxiliaryDataHash.value(name);
}

void InternalNode::setAuxiliaryData(const PropertyName &name, const QVariant &data)
{
    m_auxiliaryDataHash.insert(name, data);
}

void InternalNode::removeAuxiliaryData(const PropertyName &name)
{
    m_auxiliaryDataHash.remove(name);
}

bool InternalNode::hasAuxiliaryData(const PropertyName &name) const
{
    return m_auxiliaryDataHash.contains(name);
}

QHash<PropertyName, QVariant> InternalNode::auxiliaryData() const
{
    return m_auxiliaryDataHash;
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

InternalVariantProperty::Pointer InternalNode::variantProperty(const PropertyName &name) const
{
    InternalProperty::Pointer property =  m_namePropertyHash.value(name);
    if (property->isVariantProperty())
        return property.staticCast<InternalVariantProperty>();

    return InternalVariantProperty::Pointer();
}

void InternalNode::addBindingProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalBindingProperty::create(name, internalPointer()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addSignalHandlerProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalSignalHandlerProperty::create(name, internalPointer()));
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
    InternalProperty::Pointer newProperty(InternalVariantProperty::create(name, internalPointer()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addNodeProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalNodeProperty::create(name, internalPointer()));
    m_namePropertyHash.insert(name, newProperty);
}

void InternalNode::addNodeListProperty(const PropertyName &name)
{
    InternalProperty::Pointer newProperty(InternalNodeListProperty::create(name, internalPointer()));
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
    foreach (const InternalProperty::Pointer &property, propertyList()) {
        if (property->isNodeAbstractProperty())
            abstractPropertyList.append(property->toNodeAbstractProperty());
    }

    return abstractPropertyList;
}


QList<InternalNode::Pointer> InternalNode::allSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;
    foreach (const InternalNodeAbstractProperty::Pointer &property, nodeAbstractPropertyList()) {
        nodeList.append(property->allSubNodes());
    }

    return nodeList;
}

QList<InternalNode::Pointer> InternalNode::allDirectSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;
    foreach (const InternalNodeAbstractProperty::Pointer &property, nodeAbstractPropertyList()) {
        nodeList.append(property->directSubNodes());
    }

    return nodeList;
}

bool operator <(const InternalNode::Pointer &firstNode, const InternalNode::Pointer &secondNode)
{
    if (firstNode.isNull())
        return true;

    if (secondNode.isNull())
        return false;

    return firstNode->internalId() < secondNode->internalId();
}

void InternalNode::setScriptFunctions(const QStringList &scriptFunctionList)
{
    m_scriptFunctionList = scriptFunctionList;
}

QStringList InternalNode::scriptFunctions() const
{
    return m_scriptFunctionList;
}

qint32 InternalNode::internalId() const
{
    return m_internalId;
}

void InternalNode::setNodeSource(const QString &nodeSource)
{
    m_nodeSource = nodeSource;
}

QString InternalNode::nodeSource() const
{
    return m_nodeSource;
}

int InternalNode::nodeSourceType() const
{
    return m_nodeSourceType;
}

void InternalNode::setNodeSourceType(int i)
{
    m_nodeSourceType = i;
}

}
}
