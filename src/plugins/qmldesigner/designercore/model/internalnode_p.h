/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QMap>
#include <QHash>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QStringList>
#include "internalproperty.h"
#include "internalvariantproperty.h"
#include "internalbindingproperty.h"
#include "internalsignalhandlerproperty.h"
#include "internalnodelistproperty.h"
#include "internalnodeproperty.h"
#include "internalnodeabstractproperty.h"

#include <memory>

namespace QmlDesigner {

namespace Internal {

class InternalProperty;
class InternalNode;

using InternalNodePointer = std::shared_ptr<InternalNode>;
using InternalPropertyPointer = QSharedPointer<InternalProperty>;

class InternalNode : public std::enable_shared_from_this<InternalNode>
{
    friend InternalProperty;

public:
    using Pointer = std::shared_ptr<InternalNode>;
    using WeakPointer = std::weak_ptr<InternalNode>;

    explicit InternalNode() = default;

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

    QVariant auxiliaryData(const PropertyName &name) const;
    void setAuxiliaryData(const PropertyName &name, const QVariant &data);
    void removeAuxiliaryData(const PropertyName &name);
    bool hasAuxiliaryData(const PropertyName &name) const;
    const QHash<PropertyName, QVariant> &auxiliaryData() const;

    InternalProperty::Pointer property(const PropertyName &name) const;
    InternalBindingProperty::Pointer bindingProperty(const PropertyName &name) const;
    InternalSignalHandlerProperty::Pointer signalHandlerProperty(const PropertyName &name) const;
    InternalSignalDeclarationProperty::Pointer signalDeclarationProperty(const PropertyName &name) const;
    InternalVariantProperty::Pointer variantProperty(const PropertyName &name) const;
    InternalNodeListProperty::Pointer nodeListProperty(const PropertyName &name) const;
    InternalNodeAbstractProperty::Pointer nodeAbstractProperty(const PropertyName &name) const;
    InternalNodeProperty::Pointer nodeProperty(const PropertyName &name) const;

    void addBindingProperty(const PropertyName &name);
    void addSignalHandlerProperty(const PropertyName &name);
    void addSignalDeclarationProperty(const PropertyName &name);
    void addNodeListProperty(const PropertyName &name);
    void addVariantProperty(const PropertyName &name);
    void addNodeProperty(const PropertyName &name, const TypeName &dynamicTypeName);

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

    friend size_t qHash(const InternalNodePointer &node)
    {
        if (!node)
            return ::qHash(-1);

        return ::qHash(node->internalId);
    }

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

private:
    QHash<PropertyName, QVariant> m_auxiliaryDataHash;
    InternalNodeAbstractProperty::WeakPointer m_parentProperty;
    QHash<PropertyName, InternalPropertyPointer> m_namePropertyHash;
};

} // Internal
} // QtQmlDesigner
