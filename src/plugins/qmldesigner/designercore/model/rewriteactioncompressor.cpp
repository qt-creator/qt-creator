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

#include <QtCore/QSet>

#include "modelnode.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "qmltextgenerator.h"
#include "rewriteactioncompressor.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

static bool nodeOrParentInSet(const ModelNode &node, const QSet<ModelNode> &nodeSet)
{
    ModelNode n = node;
    while (n.isValid()) {
        if (nodeSet.contains(n))
            return true;

        if (!n.hasParentProperty())
            return false;

        n = n.parentProperty().parentModelNode();
    }

    return false;
}

void RewriteActionCompressor::operator()(QList<RewriteAction *> &actions) const
{
    compressImports(actions);
    compressReparentActions(actions);
    compressPropertyActions(actions);
    compressAddEditRemoveNodeActions(actions);
    compressAddEditActions(actions);
    compressAddReparentActions(actions);
}

void RewriteActionCompressor::compressImports(QList<RewriteAction *> &actions) const
{
    QHash<Import, RewriteAction *> addedImports;
    QHash<Import, RewriteAction *> removedImports;

    QMutableListIterator<RewriteAction *> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (RemoveImportRewriteAction const *removeImportAction = action->asRemoveImportRewriteAction()) {
            const Import import = removeImportAction->import();
            if (removedImports.contains(import)) {
                remove(iter);
            } else if (RewriteAction *addImportAction = addedImports.value(import, 0)) {
                actions.removeOne(addImportAction);
                addedImports.remove(import);
                delete addImportAction;
                remove(iter);
            } else {
                removedImports.insert(import, action);
            }
        } else if (AddImportRewriteAction const *addImportAction = action->asAddImportRewriteAction()) {
            const Import import = addImportAction->import();
            if (RewriteAction *duplicateAction = addedImports.value(import, 0)) {
                actions.removeOne(duplicateAction);
                addedImports.remove(import);
                delete duplicateAction;
                addedImports.insert(import, action);
            } else if (RewriteAction *removeAction = removedImports.value(import, 0)) {
                actions.removeOne(removeAction);
                removedImports.remove(import);
                delete removeAction;
                remove(iter);
            } else {
                addedImports.insert(import, action);
            }
        }
    }
}

void RewriteActionCompressor::compressReparentActions(QList<RewriteAction *> &actions) const
{
    QSet<ModelNode> reparentedNodes;

    QMutableListIterator<RewriteAction*> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (ReparentNodeRewriteAction const *reparentAction = action->asReparentNodeRewriteAction()) {
            const ModelNode reparentedNode = reparentAction->reparentedNode();

            if (reparentedNodes.contains(reparentedNode)) {
                remove(iter);
            } else {
                reparentedNodes.insert(reparentedNode);
            }
        }
    }
}

void RewriteActionCompressor::compressAddEditRemoveNodeActions(QList<RewriteAction *> &actions) const
{
    QHash<ModelNode, RewriteAction *> removedNodes;
    QSet<RewriteAction *> removeActionsToRemove;

    QMutableListIterator<RewriteAction*> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (RemoveNodeRewriteAction const *removeNodeAction = action->asRemoveNodeRewriteAction()) {
            const ModelNode modelNode = removeNodeAction->node();

            if (removedNodes.contains(modelNode))
                remove(iter);
            else
                removedNodes.insert(modelNode, action);
        } else if (action->asAddPropertyRewriteAction() || action->asChangePropertyRewriteAction()) {
            AbstractProperty property;
            ModelNode containedModelNode;
            if (action->asAddPropertyRewriteAction()) {
                property = action->asAddPropertyRewriteAction()->property();
                containedModelNode = action->asAddPropertyRewriteAction()->containedModelNode();
            } else {
                property = action->asChangePropertyRewriteAction()->property();
                containedModelNode = action->asChangePropertyRewriteAction()->containedModelNode();
            }

            if (removedNodes.contains(property.parentModelNode()))
                remove(iter);
            else if (removedNodes.contains(containedModelNode)) {
                remove(iter);
                removeActionsToRemove.insert(removedNodes[containedModelNode]);
            }
        } else if (RemovePropertyRewriteAction const *removePropertyAction = action->asRemovePropertyRewriteAction()) {
            const AbstractProperty property = removePropertyAction->property();

            if (removedNodes.contains(property.parentModelNode()))
                remove(iter);
        } else if (ChangeIdRewriteAction const *changeIdAction = action->asChangeIdRewriteAction()) {
            if (removedNodes.contains(changeIdAction->node()))
                remove(iter);
        } else if (ChangeTypeRewriteAction const *changeTypeAction = action->asChangeTypeRewriteAction()) {
            if (removedNodes.contains(changeTypeAction->node()))
                remove(iter);
        } else if (ReparentNodeRewriteAction const *reparentAction = action->asReparentNodeRewriteAction()) {
            if (removedNodes.contains(reparentAction->reparentedNode()))
                remove(iter);
        }
    }

    foreach (RewriteAction *action, removeActionsToRemove) {
        actions.removeOne(action);
        delete action;
    }
}

void RewriteActionCompressor::compressPropertyActions(QList<RewriteAction *> &actions) const
{
    QHash<AbstractProperty, RewriteAction *> removedProperties;
    QHash<AbstractProperty, ChangePropertyRewriteAction const*> changedProperties;
    QSet<AbstractProperty> addedProperties;

    QMutableListIterator<RewriteAction*> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (RemovePropertyRewriteAction const *removeAction = action->asRemovePropertyRewriteAction()) {
            removedProperties.insert(removeAction->property(), action);
        } else if (ChangePropertyRewriteAction const *changeAction = action->asChangePropertyRewriteAction()) {
            const AbstractProperty property = changeAction->property();

            if (removedProperties.contains(property)) {
                remove(iter);
            } else if (changedProperties.contains(property)) {
                if (!property.isValid() || !property.isDefaultProperty())
                    remove(iter);
            } else {
                changedProperties.insert(property, changeAction);
            }
        } else if (AddPropertyRewriteAction const *addAction = action->asAddPropertyRewriteAction()) {
            const AbstractProperty property = addAction->property();

            if (RewriteAction *removeAction = removedProperties.value(property, 0)) {
                actions.removeOne(removeAction);
                removedProperties.remove(property);
                delete removeAction;
                remove(iter);
            } else {
                if (changedProperties.contains(property))
                    changedProperties.remove(property);

                addedProperties.insert(property);
            }
        }
    }
}

void RewriteActionCompressor::compressAddEditActions(QList<RewriteAction *> &actions) const
{
    QSet<ModelNode> addedNodes;
    QSet<RewriteAction *> dirtyActions;

    QMutableListIterator<RewriteAction*> iter(actions);
    while (iter.hasNext()) {
        RewriteAction *action = iter.next();

        if (action->asAddPropertyRewriteAction() || action->asChangePropertyRewriteAction()) {
            AbstractProperty property;
            ModelNode containedNode;

            if (AddPropertyRewriteAction const *addAction = action->asAddPropertyRewriteAction()) {
                property = addAction->property();
                containedNode = addAction->containedModelNode();
            } else if (ChangePropertyRewriteAction const *changeAction = action->asChangePropertyRewriteAction()) {
                property = changeAction->property();
                containedNode = changeAction->containedModelNode();
            }

            if (property.isValid() && addedNodes.contains(property.parentModelNode())) {
                remove(iter);
                continue;
            }

            if (!containedNode.isValid())
                continue;

            if (nodeOrParentInSet(containedNode, addedNodes)) {
                remove(iter);
            } else {
                addedNodes.insert(containedNode);
                dirtyActions.insert(action);
            }
        } else if (ChangeIdRewriteAction const *changeIdAction = action->asChangeIdRewriteAction()) {
            if (nodeOrParentInSet(changeIdAction->node(), addedNodes)) {
                remove(iter);
            }
        } else if (ChangeTypeRewriteAction const *changeTypeAction = action->asChangeTypeRewriteAction()) {
            if (nodeOrParentInSet(changeTypeAction->node(), addedNodes)) {
                remove(iter);
            }
        }
    }

    QmlTextGenerator gen(m_propertyOrder);
    foreach (RewriteAction *action, dirtyActions) {
        RewriteAction *newAction = 0;
        if (AddPropertyRewriteAction const *addAction = action->asAddPropertyRewriteAction()) {
            newAction = new AddPropertyRewriteAction(addAction->property(),
                                                     gen(addAction->containedModelNode()),
                                                     addAction->propertyType(),
                                                     addAction->containedModelNode());
        } else if (ChangePropertyRewriteAction const *changeAction = action->asChangePropertyRewriteAction()) {
            newAction = new ChangePropertyRewriteAction(changeAction->property(),
                                                        gen(changeAction->containedModelNode()),
                                                        changeAction->propertyType(),
                                                        changeAction->containedModelNode());
        }

        const int idx = actions.indexOf(action);
        if (newAction && idx >= 0) {
            actions[idx] = newAction;
        }
    }
}

void RewriteActionCompressor::compressAddReparentActions(QList<RewriteAction *> &actions) const
{
    QMap<ModelNode, RewriteAction*> addedNodes;

    QMutableListIterator<RewriteAction*> iter(actions);
    while (iter.hasNext()) {
        RewriteAction *action = iter.next();

        if (action->asAddPropertyRewriteAction() || action->asChangePropertyRewriteAction()) {
            ModelNode containedNode;

            if (AddPropertyRewriteAction const *addAction = action->asAddPropertyRewriteAction()) {
                containedNode = addAction->containedModelNode();
            } else if (ChangePropertyRewriteAction const *changeAction = action->asChangePropertyRewriteAction()) {
                containedNode = changeAction->containedModelNode();
            }

            if (!containedNode.isValid())
                continue;

            addedNodes.insert(containedNode, action);
        } else if (ReparentNodeRewriteAction const *reparentAction = action->asReparentNodeRewriteAction()) {
            if (addedNodes.contains(reparentAction->reparentedNode())) {
                RewriteAction *previousAction = addedNodes[reparentAction->reparentedNode()];
                actions.removeOne(previousAction);

                RewriteAction *replacementAction = 0;
                if (AddPropertyRewriteAction const *addAction = previousAction->asAddPropertyRewriteAction()) {
                    replacementAction = new AddPropertyRewriteAction(reparentAction->targetProperty(),
                                                                     addAction->valueText(),
                                                                     reparentAction->propertyType(),
                                                                     addAction->containedModelNode());
                } else if (ChangePropertyRewriteAction const *changeAction = previousAction->asChangePropertyRewriteAction()) {
                    replacementAction = new AddPropertyRewriteAction(reparentAction->targetProperty(),
                                                                     changeAction->valueText(),
                                                                     reparentAction->propertyType(),
                                                                     changeAction->containedModelNode());
                }

                iter.setValue(replacementAction);
                delete previousAction;
                delete action;
            }
        }
    }
}

void RewriteActionCompressor::remove(QMutableListIterator<RewriteAction*> &iter) const
{
    delete iter.value();
    iter.remove();
}
