/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "rewriteactioncompressor.h"

#include <QSet>

#include "modelnode.h"
#include "nodelistproperty.h"
#include "qmltextgenerator.h"

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

static bool nodeOrParentInSet(const ModelNode &modelNode, const QSet<ModelNode> &nodeSet)
{
    ModelNode currentModelnode = modelNode;
    while (currentModelnode.isValid()) {
        if (nodeSet.contains(currentModelnode))
            return true;

        if (!currentModelnode.hasParentProperty())
            return false;

        currentModelnode = currentModelnode.parentProperty().parentModelNode();
    }

    return false;
}

void RewriteActionCompressor::operator()(QList<RewriteAction *> &actions) const
{
    compressImports(actions);
    compressRereparentActions(actions);
    compressReparentIntoSamePropertyActions(actions);
    compressPropertyActions(actions);
    compressAddEditRemoveNodeActions(actions);
    compressAddEditActions(actions);
    compressAddReparentActions(actions);
}

void RewriteActionCompressor::compressImports(QList<RewriteAction *> &actions) const
{
    QList<RewriteAction *> actionsToRemove;
    QHash<Import, RewriteAction *> addedImports;
    QHash<Import, RewriteAction *> removedImports;

    QMutableListIterator<RewriteAction *> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (RemoveImportRewriteAction *removeImportAction = action->asRemoveImportRewriteAction()) {
            const Import import = removeImportAction->import();
            if (removedImports.contains(import)) {
                actionsToRemove.append(action);
            } else if (RewriteAction *addImportAction = addedImports.value(import, 0)) {
                actionsToRemove.append(action);
                actionsToRemove.append(addImportAction);
                addedImports.remove(import);
                delete addImportAction;
            } else {
                removedImports.insert(import, action);
            }
        } else if (AddImportRewriteAction *addImportAction = action->asAddImportRewriteAction()) {
            const Import import = addImportAction->import();
            if (RewriteAction *duplicateAction = addedImports.value(import, 0)) {
                actionsToRemove.append(duplicateAction);
                addedImports.remove(import);
                delete duplicateAction;
                addedImports.insert(import, action);
            } else if (RewriteAction *removeAction = removedImports.value(import, 0)) {
                actionsToRemove.append(action);
                actionsToRemove.append(removeAction);
                removedImports.remove(import);
                delete removeAction;
            } else {
                addedImports.insert(import, action);
            }
        }
    }

    foreach (RewriteAction *action, actionsToRemove) {
        actions.removeOne(action);
        delete action;
    }
}

void RewriteActionCompressor::compressRereparentActions(QList<RewriteAction *> &actions) const
{
    QList<RewriteAction *> actionsToRemove;
    QHash<ModelNode, ReparentNodeRewriteAction *> reparentedNodes;

    QMutableListIterator<RewriteAction*> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (ReparentNodeRewriteAction *reparentAction = action->asReparentNodeRewriteAction()) {
            const ModelNode reparentedNode = reparentAction->reparentedNode();

            if (ReparentNodeRewriteAction *otherAction = reparentedNodes.value(reparentedNode, 0)) {
                otherAction->setOldParentProperty(reparentAction->oldParentProperty());
                actionsToRemove.append(action);
            } else {
                reparentedNodes.insert(reparentedNode, reparentAction);
            }
        }
    }

    foreach (RewriteAction *action, actionsToRemove) {
        actions.removeOne(action);
        delete action;
    }
}

void RewriteActionCompressor::compressReparentIntoSamePropertyActions(QList<RewriteAction *> &actions) const
{
    QList<RewriteAction *> actionsToRemove;
    QMutableListIterator<RewriteAction *> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (ReparentNodeRewriteAction *reparentAction = action->asReparentNodeRewriteAction()) {
            if (reparentAction->targetProperty() == reparentAction->oldParentProperty())
                actionsToRemove.append(action);
        }
    }

    foreach (RewriteAction *action, actionsToRemove) {
        actions.removeOne(action);
        delete action;
    }
}

void RewriteActionCompressor::compressAddEditRemoveNodeActions(QList<RewriteAction *> &actions) const
{
    QList<RewriteAction *> actionsToRemove;
    QHash<ModelNode, RewriteAction *> removedNodes;

    QMutableListIterator<RewriteAction*> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (RemoveNodeRewriteAction *removeNodeAction = action->asRemoveNodeRewriteAction()) {
            const ModelNode modelNode = removeNodeAction->node();

            if (removedNodes.contains(modelNode))
                actionsToRemove.append(action);
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

            if (removedNodes.contains(property.parentModelNode())) {
                actionsToRemove.append(action);
            } else if (RewriteAction *removeAction = removedNodes.value(containedModelNode, 0)) {
                actionsToRemove.append(action);
                actionsToRemove.append(removeAction);
            }
        } else if (RemovePropertyRewriteAction *removePropertyAction = action->asRemovePropertyRewriteAction()) {
            const AbstractProperty property = removePropertyAction->property();

            if (removedNodes.contains(property.parentModelNode()))
                actionsToRemove.append(action);
        } else if (ChangeIdRewriteAction *changeIdAction = action->asChangeIdRewriteAction()) {
            if (removedNodes.contains(changeIdAction->node()))
                actionsToRemove.append(action);
        } else if (ChangeTypeRewriteAction *changeTypeAction = action->asChangeTypeRewriteAction()) {
            if (removedNodes.contains(changeTypeAction->node()))
                actionsToRemove.append(action);
        } else if (ReparentNodeRewriteAction *reparentAction = action->asReparentNodeRewriteAction()) {
            if (removedNodes.contains(reparentAction->reparentedNode()))
                actionsToRemove.append(action);
        }
    }

    foreach (RewriteAction *action, actionsToRemove) {
        actions.removeOne(action);
        delete action;
    }
}

void RewriteActionCompressor::compressPropertyActions(QList<RewriteAction *> &actions) const
{
    QList<RewriteAction *> actionsToRemove;
    QHash<AbstractProperty, RewriteAction *> removedProperties;
    QHash<AbstractProperty, ChangePropertyRewriteAction *> changedProperties;
    QHash<AbstractProperty, AddPropertyRewriteAction *> addedProperties;

    QMutableListIterator<RewriteAction*> iter(actions);
    iter.toBack();
    while (iter.hasPrevious()) {
        RewriteAction *action = iter.previous();

        if (RemovePropertyRewriteAction *removeAction = action->asRemovePropertyRewriteAction()) {
            const AbstractProperty property = removeAction->property();
            if (AddPropertyRewriteAction *addAction = addedProperties.value(property, 0)) {
                Q_UNUSED(addAction);
            } else {
                removedProperties.insert(property, action);
            }
        } else if (ChangePropertyRewriteAction *changeAction = action->asChangePropertyRewriteAction()) {
            const AbstractProperty property = changeAction->property();

            if (removedProperties.contains(property)) {
                actionsToRemove.append(action);
            } else if (changedProperties.contains(property)) {
                if (!property.isValid() || !property.isDefaultProperty())
                    actionsToRemove.append(action);
            } else {
                changedProperties.insert(property, changeAction);
            }
        } else if (AddPropertyRewriteAction *addAction = action->asAddPropertyRewriteAction()) {
            const AbstractProperty property = addAction->property();

            if (RewriteAction *removeAction = removedProperties.value(property, 0)) {
                actionsToRemove.append(action);
                actionsToRemove.append(removeAction);
                removedProperties.remove(property);
            } else {
                if (changedProperties.contains(property))
                    changedProperties.remove(property);

                addedProperties.insert(property, addAction);
            }
        }
    }

    foreach (RewriteAction *action, actionsToRemove){
        actions.removeOne(action);
        delete action;
    }
}

void RewriteActionCompressor::compressAddEditActions(QList<RewriteAction *> &actions) const
{
    QList<RewriteAction *> actionsToRemove;
    QSet<ModelNode> addedNodes;
    QSet<RewriteAction *> dirtyActions;

    QMutableListIterator<RewriteAction*> iter(actions);
    while (iter.hasNext()) {
        RewriteAction *action = iter.next();

        if (action->asAddPropertyRewriteAction() || action->asChangePropertyRewriteAction()) {
            AbstractProperty property;
            ModelNode containedNode;

            if (AddPropertyRewriteAction *addAction = action->asAddPropertyRewriteAction()) {
                property = addAction->property();
                containedNode = addAction->containedModelNode();
            } else if (ChangePropertyRewriteAction *changeAction = action->asChangePropertyRewriteAction()) {
                property = changeAction->property();
                containedNode = changeAction->containedModelNode();
            }

            if (property.isValid() && addedNodes.contains(property.parentModelNode())) {
                actionsToRemove.append(action);
                continue;
            }

            if (!containedNode.isValid())
                continue;

            if (nodeOrParentInSet(containedNode, addedNodes)) {
                actionsToRemove.append(action);
            } else {
                addedNodes.insert(containedNode);
                dirtyActions.insert(action);
            }
        } else if (ChangeIdRewriteAction *changeIdAction = action->asChangeIdRewriteAction()) {
            if (nodeOrParentInSet(changeIdAction->node(), addedNodes))
                actionsToRemove.append(action);
        } else if (ChangeTypeRewriteAction *changeTypeAction = action->asChangeTypeRewriteAction()) {
            if (nodeOrParentInSet(changeTypeAction->node(), addedNodes))
                actionsToRemove.append(action);
        }
    }

    foreach (RewriteAction *action, actionsToRemove){
        actions.removeOne(action);
        delete action;
    }

    QmlTextGenerator gen(m_propertyOrder);
    foreach (RewriteAction *action, dirtyActions) {
        RewriteAction *newAction = 0;
        if (AddPropertyRewriteAction *addAction = action->asAddPropertyRewriteAction()) {
            newAction = new AddPropertyRewriteAction(addAction->property(),
                                                     gen(addAction->containedModelNode()),
                                                     addAction->propertyType(),
                                                     addAction->containedModelNode());
        } else if (ChangePropertyRewriteAction *changeAction = action->asChangePropertyRewriteAction()) {
            newAction = new ChangePropertyRewriteAction(changeAction->property(),
                                                        gen(changeAction->containedModelNode()),
                                                        changeAction->propertyType(),
                                                        changeAction->containedModelNode());
        }

        const int idx = actions.indexOf(action);
        if (newAction && idx >= 0)
            actions[idx] = newAction;
        else
            delete newAction;
    }
}

void RewriteActionCompressor::compressAddReparentActions(QList<RewriteAction *> &actions) const
{
    QList<RewriteAction *> actionsToRemove;
    QMap<ModelNode, RewriteAction*> addedNodes;

    QMutableListIterator<RewriteAction*> iter(actions);
    while (iter.hasNext()) {
        RewriteAction *action = iter.next();

        if (action->asAddPropertyRewriteAction() || action->asChangePropertyRewriteAction()) {
            ModelNode containedNode;

            if (AddPropertyRewriteAction *addAction = action->asAddPropertyRewriteAction())
                containedNode = addAction->containedModelNode();
            else if (ChangePropertyRewriteAction *changeAction = action->asChangePropertyRewriteAction())
                containedNode = changeAction->containedModelNode();

            if (!containedNode.isValid())
                continue;

            addedNodes.insert(containedNode, action);
        } else if (ReparentNodeRewriteAction *reparentAction = action->asReparentNodeRewriteAction()) {
            if (addedNodes.contains(reparentAction->reparentedNode())) {
                RewriteAction *previousAction = addedNodes[reparentAction->reparentedNode()];
                actionsToRemove.append(previousAction);

                RewriteAction *replacementAction = 0;
                if (AddPropertyRewriteAction *addAction = previousAction->asAddPropertyRewriteAction()) {
                    replacementAction = new AddPropertyRewriteAction(reparentAction->targetProperty(),
                                                                     addAction->valueText(),
                                                                     reparentAction->propertyType(),
                                                                     addAction->containedModelNode());
                } else if (ChangePropertyRewriteAction *changeAction = previousAction->asChangePropertyRewriteAction()) {
                    replacementAction = new AddPropertyRewriteAction(reparentAction->targetProperty(),
                                                                     changeAction->valueText(),
                                                                     reparentAction->propertyType(),
                                                                     changeAction->containedModelNode());
                }

                iter.setValue(replacementAction);
                delete action;
            }
        }
    }

    foreach (RewriteAction *action, actionsToRemove){
        actions.removeOne(action);
        delete action;
    }
}
