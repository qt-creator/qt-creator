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

namespace QmlDesigner {

class Model;

namespace Internal {

class InternalProperty;
class InternalNode;

typedef QSharedPointer<InternalNode> InternalNodePointer;
typedef QSharedPointer<InternalProperty> InternalPropertyPointer;
typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;

class InternalNode
{
    friend class InternalProperty;
public:
    typedef QSharedPointer<InternalNode> Pointer;
    typedef QWeakPointer<InternalNode> WeakPointer;

    explicit InternalNode();

    static Pointer create(const TypeName &typeName, int majorVersion, int minorVersion, qint32 internalId);

    TypeName type() const;
    void setType(const TypeName &newType);

    int minorVersion() const;
    int majorVersion() const;
    void setMinorVersion(int number);
    void setMajorVersion(int number);

    bool isValid() const;
    void setValid(bool valid);

    InternalNodeAbstractProperty::Pointer parentProperty() const;

    // Reparent within model
    void setParentProperty(const InternalNodeAbstractProperty::Pointer &parent);
    void resetParentProperty();

    QString id() const;
    void setId(const QString& id);
    bool hasId() const;

    QVariant auxiliaryData(const PropertyName &name) const;
    void setAuxiliaryData(const PropertyName &name, const QVariant &data);
    void removeAuxiliaryData(const PropertyName &name);
    bool hasAuxiliaryData(const PropertyName &name) const;
    QHash<PropertyName, QVariant> auxiliaryData() const;

    InternalProperty::Pointer property(const PropertyName &name) const;
    InternalBindingProperty::Pointer bindingProperty(const PropertyName &name) const;
    InternalSignalHandlerProperty::Pointer signalHandlerProperty(const PropertyName &name) const;
    InternalVariantProperty::Pointer variantProperty(const PropertyName &name) const;
    InternalNodeListProperty::Pointer nodeListProperty(const PropertyName &name) const;
    InternalNodeAbstractProperty::Pointer nodeAbstractProperty(const PropertyName &name) const;
    InternalNodeProperty::Pointer nodeProperty(const PropertyName &name) const;

    void addBindingProperty(const PropertyName &name);
    void addSignalHandlerProperty(const PropertyName &name);
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

    void setScriptFunctions(const QStringList &scriptFunctionList);
    QStringList scriptFunctions() const;

    qint32 internalId() const;

    void setNodeSource(const QString&);
    QString nodeSource() const;

    int nodeSourceType() const;
    void setNodeSourceType(int i);

protected:
    Pointer internalPointer() const;
    void setInternalWeakPointer(const Pointer &pointer);
    void removeProperty(const PropertyName &name);
    explicit InternalNode(const TypeName &type, int majorVersion, int minorVersion, qint32 internalId);

private:
    TypeName m_typeName;
    QString m_id;

    QHash<PropertyName, QVariant> m_auxiliaryDataHash;

    InternalNodeAbstractProperty::WeakPointer m_parentProperty;
    WeakPointer m_internalPointer;

    int m_majorVersion;
    int m_minorVersion;

    bool m_valid;
    qint32 m_internalId;

    QHash<PropertyName, InternalPropertyPointer> m_namePropertyHash;
    QStringList m_scriptFunctionList;

    QString m_nodeSource;
    int m_nodeSourceType;
};

uint qHash(const InternalNodePointer& node);
bool operator <(const InternalNodePointer &firstNode, const InternalNodePointer &secondNode);
} // Internal
} // QtQmlDesigner
