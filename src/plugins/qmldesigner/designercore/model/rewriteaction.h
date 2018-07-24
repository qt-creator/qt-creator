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

    virtual AddImportRewriteAction *asAddImportRewriteAction() { return nullptr; }
    virtual AddPropertyRewriteAction *asAddPropertyRewriteAction() { return nullptr; }
    virtual ChangeIdRewriteAction *asChangeIdRewriteAction() { return nullptr; }
    virtual ChangePropertyRewriteAction *asChangePropertyRewriteAction() { return nullptr; }
    virtual ChangeTypeRewriteAction *asChangeTypeRewriteAction() { return nullptr; }
    virtual RemoveImportRewriteAction *asRemoveImportRewriteAction() { return nullptr; }
    virtual RemoveNodeRewriteAction *asRemoveNodeRewriteAction() { return nullptr; }
    virtual RemovePropertyRewriteAction *asRemovePropertyRewriteAction() { return nullptr; }
    virtual ReparentNodeRewriteAction *asReparentNodeRewriteAction() { return nullptr; }
    virtual MoveNodeRewriteAction *asMoveNodeRewriteAction() { return nullptr; }
    virtual ~RewriteAction() = default;

protected:
    RewriteAction() = default;

public:
    RewriteAction(const RewriteAction&) = delete;
    RewriteAction &operator=(const RewriteAction&) = delete;
};

class AddPropertyRewriteAction: public RewriteAction
{
public:
    AddPropertyRewriteAction(const AbstractProperty &property, const QString &valueText, QmlDesigner::QmlRefactoring::PropertyType propertyType, const ModelNode &containedModelNode/* = ModelNode()*/):
            m_property(property), m_valueText(valueText), m_propertyType(propertyType), m_containedModelNode(containedModelNode),
            m_sheduledInHierarchy(property.isValid() && property.parentModelNode().isInHierarchy())
    {}

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    AddPropertyRewriteAction *asAddPropertyRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    ChangeIdRewriteAction *asChangeIdRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    ChangePropertyRewriteAction *asChangePropertyRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    ChangeTypeRewriteAction *asChangeTypeRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    RemoveNodeRewriteAction *asRemoveNodeRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    RemovePropertyRewriteAction *asRemovePropertyRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    ReparentNodeRewriteAction *asReparentNodeRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    MoveNodeRewriteAction *asMoveNodeRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    AddImportRewriteAction *asAddImportRewriteAction() override { return this; }

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

    bool execute(QmlDesigner::QmlRefactoring &refactoring, ModelNodePositionStorage &positionStore) override;
    QString info() const override;

    RemoveImportRewriteAction *asRemoveImportRewriteAction() override { return this; }

    Import import() const { return m_import; }

private:
    Import m_import;
};

} // namespace Internal
} // namespace QmlDesigner
