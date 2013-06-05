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

#ifndef QMLOBJECTNODE_H
#define QMLOBJECTNODE_H

#include <qmldesignercorelib_global.h>
#include "qmlmodelnodefacade.h"
#include "qmlstate.h"
#include "qmlchangeset.h"

#include <nodeinstance.h>

namespace QmlDesigner {

class QmlItemNode;
class QmlPropertyChanges;

class QMLDESIGNERCORE_EXPORT QmlObjectNode : public QmlModelNodeFacade
{
public:
    QmlObjectNode() : QmlModelNodeFacade() {}
    QmlObjectNode(const ModelNode &modelNode)  : QmlModelNodeFacade(modelNode) {}

    bool hasNodeParent() const;
    bool hasInstanceParent() const;
    bool hasInstanceParentItem() const;
    void setParentProperty(const NodeAbstractProperty &parentProeprty);
    QmlObjectNode instanceParent() const;
    QmlItemNode instanceParentItem() const;

    void setId(const QString &id);
    QString id() const;
    QString validId();

    QmlModelState currentState() const;
    void setVariantProperty(const PropertyName &name, const QVariant &value);
    void setBindingProperty(const PropertyName &name, const QString &expression);
    NodeAbstractProperty nodeAbstractProperty(const PropertyName &name) const;
    NodeProperty nodeProperty(const PropertyName &name) const;
    NodeListProperty nodeListProperty(const PropertyName &name) const;

    QVariant instanceValue(const PropertyName &name) const;
    TypeName instanceType(const PropertyName &name) const;

    bool hasProperty(const PropertyName &name) const;
    bool hasBindingProperty(const PropertyName &name) const;
    bool instanceHasBinding(const PropertyName &name) const;
    bool propertyAffectedByCurrentState(const PropertyName &name) const;
    QVariant modelValue(const PropertyName &name) const;
    bool isTranslatableText(const PropertyName &name) const;
    QString stripedTranslatableText(const PropertyName &name) const;
    QString expression(const PropertyName &name) const;
    bool isInBaseState() const;
    QmlPropertyChanges propertyChangeForCurrentState() const;

    virtual bool canReparent() const;

    bool isRootModelNode() const;

    void destroy();

    QList<QmlModelState> allAffectingStates() const;
    QList<QmlModelStateOperation> allAffectingStatesOperations() const;

    void removeVariantProperty(const PropertyName &name);

    void setParent(QmlObjectNode newParent);

    QmlItemNode toQmlItemNode() const;

    bool isAncestorOf(const QmlObjectNode &objectNode) const;

    bool hasDefaultProperty() const;
    PropertyName defaultProperty() const;

    static  QVariant instanceValue(const ModelNode &modelNode, const PropertyName &name);

    static QString generateTranslatableText(const QString& text);

protected:
    NodeInstance nodeInstance() const;
    QmlObjectNode nodeForInstance(const NodeInstance &instance) const;
    QmlItemNode itemForInstance(const NodeInstance &instance) const;

protected:
    QList<QmlModelState> allDefinedStates() const;
};

QMLDESIGNERCORE_EXPORT uint qHash(const QmlObjectNode &node);
QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlObjectNode> &fxObjectNodeList);
QMLDESIGNERCORE_EXPORT QList<QmlObjectNode> toQmlObjectNodeList(const QList<ModelNode> &modelNodeList);
}// QmlDesigner

#endif // QMLOBJECTNODE_H
