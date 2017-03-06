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

#include "abstractproperty.h"
#include "modelnodepositionstorage.h"

#include <filemanager/qmlrefactoring.h>

namespace QmlDesigner {
namespace Internal {

class AddImportRewriteAction;
class AddPropertyRewriteAction;
class ChangeIdRewriteAction;
class ChangePropertyRewriteAction;
class ChangeTypeRewriteAction;
class RemoveImportRewriteAction;
class RemoveNodeRewriteAction;
class RemovePropertyRewriteAction;
class ReparentNodeRewriteAction;
class MoveNodeRewriteAction;

class RewriteAction
{
public:
    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) = 0;
    virtual QString info() const = 0;

    virtual AddImportRewriteAction *asAddImportRewriteAction() { return 0; }
    virtual AddPropertyRewriteAction *asAddPropertyRewriteAction() { return 0; }
    virtual ChangeIdRewriteAction *asChangeIdRewriteAction() { return 0; }
    virtual ChangePropertyRewriteAction *asChangePropertyRewriteAction() { return 0; }
    virtual ChangeTypeRewriteAction *asChangeTypeRewriteAction() { return 0; }
    virtual RemoveImportRewriteAction * asRemoveImportRewriteAction() { return 0; }
    virtual RemoveNodeRewriteAction *asRemoveNodeRewriteAction() { return 0; }
    virtual RemovePropertyRewriteAction *asRemovePropertyRewriteAction() { return 0; }
    virtual ReparentNodeRewriteAction *asReparentNodeRewriteAction() { return 0; }
    virtual MoveNodeRewriteAction *asMoveNodeRewriteAction() { return 0; }
    virtual ~RewriteAction() {}

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
            m_property(property), m_valueText(valueText), m_propertyType(propertyType), m_containedModelNode(containedModelNode),
            m_sheduledInHierarchy(property.isValid() && property.parentModelNode().isInHierarchy())
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual QString info() const;

    virtual AddPropertyRewriteAction *asAddPropertyRewriteAction() { return this; }

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
    bool m_sheduledInHierarchy;
};

class ChangeIdRewriteAction: public RewriteAction
{
public:
    ChangeIdRewriteAction(const ModelNode &node, const QString &oldId, const QString &newId):
            m_node(node), m_oldId(oldId), m_newId(newId)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual QString info() const;

    virtual ChangeIdRewriteAction *asChangeIdRewriteAction() { return this; }

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
            m_property(property), m_valueText(valueText), m_propertyType(propertyType), m_containedModelNode(containedModelNode),
            m_sheduledInHierarchy(property.isValid() && property.parentModelNode().isInHierarchy())
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual QString info() const;

    virtual ChangePropertyRewriteAction *asChangePropertyRewriteAction() { return this; }

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
    bool m_sheduledInHierarchy;
};

class ChangeTypeRewriteAction:public RewriteAction
{
public:
    ChangeTypeRewriteAction(const ModelNode &node):
            m_node(node)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual QString info() const;

    virtual ChangeTypeRewriteAction *asChangeTypeRewriteAction() { return this; }

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
    virtual QString info() const;

    virtual RemoveNodeRewriteAction *asRemoveNodeRewriteAction() { return this; }

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
    virtual QString info() const;

    virtual RemovePropertyRewriteAction *asRemovePropertyRewriteAction() { return this; }

    AbstractProperty property() const
    { return m_property; }

private:
    AbstractProperty m_property;
};

class ReparentNodeRewriteAction: public RewriteAction
{
public:
    ReparentNodeRewriteAction(const ModelNode &node, const AbstractProperty &oldParentProperty, const AbstractProperty &targetProperty, QmlDesigner::QmlRefactoring::PropertyType propertyType):
            m_node(node), m_oldParentProperty(oldParentProperty), m_targetProperty(targetProperty), m_propertyType(propertyType)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual QString info() const;

    virtual ReparentNodeRewriteAction *asReparentNodeRewriteAction() { return this; }

    ModelNode reparentedNode() const
    { return m_node; }

    void setOldParentProperty(const AbstractProperty &oldParentProperty)
    { m_oldParentProperty = oldParentProperty; }

    AbstractProperty oldParentProperty() const
    { return m_oldParentProperty; }

    AbstractProperty targetProperty() const
    { return m_targetProperty; }

    QmlDesigner::QmlRefactoring::PropertyType propertyType() const
    { return m_propertyType; }

private:
    ModelNode m_node;
    AbstractProperty m_oldParentProperty;
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
    virtual QString info() const;

    virtual MoveNodeRewriteAction *asMoveNodeRewriteAction() { return this; }

private:
    ModelNode m_movingNode;
    ModelNode m_newTrailingNode;
};

class AddImportRewriteAction: public RewriteAction
{
public:
    AddImportRewriteAction(const Import &import):
            m_import(import)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual QString info() const;

    virtual AddImportRewriteAction *asAddImportRewriteAction() { return this; }

    Import import() const { return m_import; }

private:
    Import m_import;
};

class RemoveImportRewriteAction: public RewriteAction
{
public:
    RemoveImportRewriteAction(const Import &import):
            m_import(import)
    {}

    virtual bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore);
    virtual QString info() const;

    virtual RemoveImportRewriteAction *asRemoveImportRewriteAction() { return this; }

    Import import() const { return m_import; }

private:
    Import m_import;
};

} // namespace Internal
} // namespace QmlDesigner
