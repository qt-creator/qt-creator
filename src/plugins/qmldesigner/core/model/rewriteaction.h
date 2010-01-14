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

#ifndef REWRITEACTION_H
#define REWRITEACTION_H

#include "abstractproperty.h"
#include "modelnodepositionstorage.h"

#include <filemanager/qmlrefactoring.h>

namespace QmlDesigner {
namespace Internal {

class AddPropertyRewriteAction;
class ChangeIdRewriteAction;
class ChangePropertyRewriteAction;
class ChangeTypeRewriteAction;
class RemoveNodeRewriteAction;
class RemovePropertyRewriteAction;
class ReparentNodeRewriteAction;
class MoveNodeRewriteAction;

class RewriteAction
{
public:
    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) = 0;
    virtual void dump(const QString &prefix) const = 0;

    virtual AddPropertyRewriteAction const *asAddPropertyRewriteAction() const { return 0; }
    virtual ChangeIdRewriteAction const *asChangeIdRewriteAction() const { return 0; }
    virtual ChangePropertyRewriteAction const *asChangePropertyRewriteAction() const { return 0; }
    virtual ChangeTypeRewriteAction const *asChangeTypeRewriteAction() const { return 0; }
    virtual RemoveNodeRewriteAction const *asRemoveNodeRewriteAction() const { return 0; }
    virtual RemovePropertyRewriteAction const *asRemovePropertyRewriteAction() const { return 0; }
    virtual ReparentNodeRewriteAction const *asReparentNodeRewriteAction() const { return 0; }
    virtual MoveNodeRewriteAction const *asMoveNodeRewriteAction() const { return 0; }

protected:
    RewriteAction()
    {}

private:
    RewriteAction(const RewriteAction &);
    RewriteAction &operator=(const RewriteAction&);
};

class AddPropertyRewriteAction: public RewriteAction
{
public:
    AddPropertyRewriteAction(const AbstractProperty &property, const QString &valueText, QmlDesigner::QmlRefactoring::PropertyType propertyType, const ModelNode &containedModelNode/* = ModelNode()*/):
            m_property(property), m_valueText(valueText), m_propertyType(propertyType), m_containedModelNode(containedModelNode)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual void dump(const QString &prefix) const;

    virtual AddPropertyRewriteAction const *asAddPropertyRewriteAction() const { return this; }

    AbstractProperty property() const
    { return m_property; }

    QString valueText() const
    { return m_valueText; }

    QmlDesigner::QmlRefactoring::PropertyType propertyType() const
    { return m_propertyType; }

    ModelNode containedModelNode() const
    { return m_containedModelNode; }

private:
    AbstractProperty m_property;
    QString m_valueText;
    QmlDesigner::QmlRefactoring::PropertyType m_propertyType;
    ModelNode m_containedModelNode;
};

class ChangeIdRewriteAction: public RewriteAction
{
public:
    ChangeIdRewriteAction(const ModelNode &node, const QString &oldId, const QString &newId):
            m_node(node), m_oldId(oldId), m_newId(newId)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual void dump(const QString &prefix) const;

    virtual ChangeIdRewriteAction const *asChangeIdRewriteAction() const { return this; }

    ModelNode node() const
    { return m_node; }

private:
    ModelNode m_node;
    QString m_oldId;
    QString m_newId;
};

class ChangePropertyRewriteAction: public RewriteAction
{
public:
    ChangePropertyRewriteAction(const AbstractProperty &property, const QString &valueText, QmlDesigner::QmlRefactoring::PropertyType propertyType, const ModelNode &containedModelNode/* = ModelNode()*/):
            m_property(property), m_valueText(valueText), m_propertyType(propertyType), m_containedModelNode(containedModelNode)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual void dump(const QString &prefix) const;

    virtual ChangePropertyRewriteAction const *asChangePropertyRewriteAction() const { return this; }

    AbstractProperty property() const
    { return m_property; }

    QString valueText() const
    { return m_valueText; }

    QmlDesigner::QmlRefactoring::PropertyType propertyType() const
    { return m_propertyType; }

    ModelNode containedModelNode() const
    { return m_containedModelNode; }

private:
    AbstractProperty m_property;
    QString m_valueText;
    QmlDesigner::QmlRefactoring::PropertyType m_propertyType;
    ModelNode m_containedModelNode;
};

class ChangeTypeRewriteAction:public RewriteAction
{
public:
    ChangeTypeRewriteAction(const ModelNode &node):
            m_node(node)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual void dump(const QString &prefix) const;

    virtual ChangeTypeRewriteAction const *asChangeTypeRewriteAction() const { return this; }

    ModelNode node() const
    { return m_node; }

private:
    ModelNode m_node;
};

class RemoveNodeRewriteAction: public RewriteAction
{
public:
    RemoveNodeRewriteAction(const ModelNode &node):
            m_node(node)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual void dump(const QString &prefix) const;

    virtual RemoveNodeRewriteAction const *asRemoveNodeRewriteAction() const { return this; }

    ModelNode node() const
    { return m_node; }

private:
    ModelNode m_node;
};

class RemovePropertyRewriteAction: public RewriteAction
{
public:
    RemovePropertyRewriteAction(const AbstractProperty &property):
            m_property(property)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual void dump(const QString &prefix) const;

    virtual RemovePropertyRewriteAction const *asRemovePropertyRewriteAction() const { return this; }

    AbstractProperty property() const
    { return m_property; }

private:
    AbstractProperty m_property;
};

class ReparentNodeRewriteAction: public RewriteAction
{
public:
    ReparentNodeRewriteAction(const ModelNode &node, const AbstractProperty &targetProperty, QmlDesigner::QmlRefactoring::PropertyType propertyType):
            m_node(node), m_targetProperty(targetProperty), m_propertyType(propertyType)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual void dump(const QString &prefix) const;

    virtual ReparentNodeRewriteAction const *asReparentNodeRewriteAction() const { return this; }

    ModelNode reparentedNode() const
    { return m_node; }

    AbstractProperty targetProperty() const
    { return m_targetProperty; }

    QmlDesigner::QmlRefactoring::PropertyType propertyType() const
    { return m_propertyType; }

private:
    ModelNode m_node;
    AbstractProperty m_targetProperty;
    QmlDesigner::QmlRefactoring::PropertyType m_propertyType;
};

class MoveNodeRewriteAction: public RewriteAction
{
public:
    MoveNodeRewriteAction(const ModelNode &movingNode, const ModelNode &newTrailingNode):
            m_movingNode(movingNode), m_newTrailingNode(newTrailingNode)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual void dump(const QString &prefix) const;

    virtual MoveNodeRewriteAction const *asMoveNodeRewriteAction() const { return this; }

private:
    ModelNode m_movingNode;
    ModelNode m_newTrailingNode;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // REWRITEACTION_H
