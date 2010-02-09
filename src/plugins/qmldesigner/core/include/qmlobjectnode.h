/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef FXOBJECTNODE_H
#define FXOBJECTNODE_H

#include <corelib_global.h>
#include "qmlmodelnodefacade.h"
#include "qmlstate.h"
#include "qmlchangeset.h"

namespace QmlDesigner {

class QmlItemNode;
class QmlPropertyChanges;

class CORESHARED_EXPORT QmlObjectNode : public QmlModelNodeFacade
{
public:
    QmlObjectNode() : QmlModelNodeFacade() {}
    QmlObjectNode(const ModelNode &modelNode)  : QmlModelNodeFacade(modelNode) {}

    bool hasNodeParent() const;
    bool hasInstanceParent() const;
    void setParentProperty(const NodeAbstractProperty &parentProeprty);
    QmlObjectNode instanceParent() const;

    void setId(const QString &id);
    QString id() const;
    QString validId();

    QmlModelState currentState() const;
    void setVariantProperty(const QString &name, const QVariant &value);
    void setBindingProperty(const QString &name, const QString &expression);
    NodeAbstractProperty nodeAbstractProperty(const QString &name) const;
    NodeProperty nodeProperty(const QString &name) const;
    NodeListProperty nodeListProperty(const QString &name) const;

    QVariant instanceValue(const QString &name) const;

    bool hasProperty(const QString &name) const;
    bool hasBindingProperty(const QString &name) const;
    bool propertyAffectedByCurrentState(const QString &name) const;
    QVariant modelValue(const QString &name) const;
    bool isInBaseState() const;
    QmlPropertyChanges propertyChangeForCurrentState() const;

    virtual bool canReparent() const;

    bool isRootModelNode() const;

    void destroy();

    QList<QmlModelState> allAffectingStates() const;
    QList<QmlModelStateOperation> allAffectingStatesOperations() const;

    void removeVariantProperty(const QString &name);

    void setParent(QmlObjectNode newParent);

    QmlItemNode toQmlItemNode() const;

    bool isAncestorOf(const QmlObjectNode &objectNode) const;

protected:
    NodeInstance nodeInstance() const;
    QmlObjectNode nodeForInstance(const NodeInstance &instance) const;

protected:
    QList<QmlModelState> allDefinedStates() const;
};

CORESHARED_EXPORT uint qHash(const QmlObjectNode &node);
CORESHARED_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlObjectNode> &fxObjectNodeList);
CORESHARED_EXPORT QList<QmlObjectNode> toQmlObjectNodeList(const QList<ModelNode> &modelNodeList);
}// QmlDesigner

#endif // FXOBJECTNODE_H
