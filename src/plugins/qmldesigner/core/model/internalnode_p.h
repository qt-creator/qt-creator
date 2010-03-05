/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef INTERNALNODE_H
#define INTERNALNODE_H

#include <QMap>
#include <QHash>
#include <QSharedPointer>
#include <QWeakPointer>
#include "internalproperty.h"
#include "internalvariantproperty.h"
#include "internalbindingproperty.h"
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

    static Pointer create(const QString &typeName, int majorVersion, int minorVersion);

    QString type() const;
    void setType(const QString &newType);

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

    QVariant auxiliaryData(const QString &name) const;
    void setAuxiliaryData(const QString &name, const QVariant &data);
    bool hasAuxiliaryData(const QString &name) const;

    InternalProperty::Pointer property(const QString &name) const;
    InternalBindingProperty::Pointer bindingProperty(const QString &name) const;
    InternalVariantProperty::Pointer variantProperty(const QString &name) const;
    InternalNodeListProperty::Pointer nodeListProperty(const QString &name) const;
    InternalNodeAbstractProperty::Pointer nodeAbstractProperty(const QString &name) const;
    InternalNodeProperty::Pointer nodeProperty(const QString &name) const;

    void addBindingProperty(const QString &name);
    void addNodeListProperty(const QString &name);
    void addVariantProperty(const QString &name);
    void addNodeProperty(const QString &name);


    QList<QString> propertyNameList() const;

    bool hasProperties() const;
    bool hasProperty(const QString &name) const;

    QList<InternalProperty::Pointer> propertyList() const;
    QList<InternalNodeAbstractProperty::Pointer> nodeAbstractPropertyList() const;
    QList<InternalNode::Pointer> allSubNodes() const;
    QList<InternalNode::Pointer> allDirectSubNodes() const;

protected:
    Pointer internalPointer() const;
    void setInternalWeakPointer(const Pointer &pointer);
    void removeProperty(const QString &name);
    explicit InternalNode(const QString &type, int majorVersion, int minorVersion);

private:
    QString m_typeName;
    QString m_id;

    QHash<QString, QVariant> m_auxiliaryDataHash;

    InternalNodeAbstractProperty::WeakPointer m_parentProperty;
    WeakPointer m_internalPointer;

    int m_majorVersion;
    int m_minorVersion;

    bool m_valid;

    QHash<QString, InternalPropertyPointer> m_namePropertyHash;
};

uint qHash(const InternalNodePointer& node);
bool operator <(const InternalNodePointer &firstNode, const InternalNodePointer &secondNode);

} // Internal
} // QtQmlDesigner

#endif // INTERNALNODE_H
