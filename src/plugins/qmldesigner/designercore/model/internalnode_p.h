/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef INTERNALNODE_H
#define INTERNALNODE_H

#include <QMap>
#include <QHash>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QStringList>
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

    static Pointer create(const QString &typeName, int majorVersion, int minorVersion, qint32 internalId);

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
    QHash<QString, QVariant> auxiliaryData() const;

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
    void removeProperty(const QString &name);
    explicit InternalNode(const QString &type, int majorVersion, int minorVersion, qint32 internalId);

private:
    QString m_typeName;
    QString m_id;

    QHash<QString, QVariant> m_auxiliaryDataHash;

    InternalNodeAbstractProperty::WeakPointer m_parentProperty;
    WeakPointer m_internalPointer;

    int m_majorVersion;
    int m_minorVersion;

    bool m_valid;
    qint32 m_internalId;

    QHash<QString, InternalPropertyPointer> m_namePropertyHash;
    QStringList m_scriptFunctionList;

    QString m_nodeSource;
    int m_nodeSourceType;
};

uint qHash(const InternalNodePointer& node);
bool operator <(const InternalNodePointer &firstNode, const InternalNodePointer &secondNode);
} // Internal
} // QtQmlDesigner

#endif // INTERNALNODE_H
