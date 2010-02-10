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

#include "modelmerger.h"

#include "modelmerger.h"
#include "modelnode.h"
#include "abstractview.h"
#include "model.h"
#include "nodeproperty.h"
#include "nodelistproperty.h"
#include "bindingproperty.h"
#include "variantproperty.h"
#include "rewritertransaction.h"

#include <QSet>
#include <QStringList>

#include <QtDebug>

namespace QmlDesigner {

static ModelNode createNodeFromNode(const ModelNode &modelNode,const QHash<QString, QString> &idRenamingHash, AbstractView *view);

static QString fixExpression(const QString &expression, const QHash<QString, QString> &idRenamingHash)
{
    QString newExpression = expression;
    foreach (const QString &id, idRenamingHash.keys()) {
        if (newExpression.contains(id))
            newExpression = newExpression.replace(id, idRenamingHash.value(id));
    }
    return newExpression;
}

static void syncVariantProperties(ModelNode &outputNode, const ModelNode &inputNode)
{
    foreach (const VariantProperty &variantProperty, inputNode.variantProperties()) {
        outputNode.variantProperty(variantProperty.name()) = variantProperty.value();
    }
}

static void syncBindingProperties(ModelNode &outputNode, const ModelNode &inputNode, const QHash<QString, QString> &idRenamingHash)
{
    foreach (const BindingProperty &bindingProperty, inputNode.bindingProperties()) {
        outputNode.bindingProperty(bindingProperty.name()).setExpression(fixExpression(bindingProperty.expression(), idRenamingHash));
    }
}

static void syncId(ModelNode &outputNode, const ModelNode &inputNode, const QHash<QString, QString> &idRenamingHash)
{
    if (!inputNode.id().isEmpty()) {
        outputNode.setId(idRenamingHash.value(inputNode.id()));
    }
}

static void splitIdInBaseNameAndNumber(const QString &id, QString *baseId, int *number)
{

    int counter = 0;
    while(counter < id.count()) {
        bool canConvertToInteger = false;
        int newNumber = id.right(counter +1).toInt(&canConvertToInteger);
        if (canConvertToInteger)
            *number = newNumber;
        else
            break;

        counter++;
    }

    *baseId = id.left(id.count() - counter);
}

static void setupIdRenamingHash(const ModelNode &modelNode, QHash<QString, QString> &idRenamingHash, AbstractView *view)
{
    QList<ModelNode> allNodes(modelNode.allSubModelNodes());
    allNodes.append(modelNode);
    foreach (const ModelNode &node, allNodes) {
        if (!node.id().isEmpty()) {
            QString newId = node.id();
            QString baseId;
            int number = 1;
            splitIdInBaseNameAndNumber(newId, &baseId, &number);

            while (view->hasId(newId) || idRenamingHash.contains(newId)) {
                newId = baseId + QString::number(number);
                number++;
            }

            idRenamingHash.insert(node.id(), newId);
        }
    }
}

static void syncNodeProperties(ModelNode &outputNode, const ModelNode &inputNode, const QHash<QString, QString> &idRenamingHash, AbstractView *view)
{
    foreach (const NodeProperty &nodeProperty, inputNode.nodeProperties()) {
        ModelNode newNode = createNodeFromNode(nodeProperty.modelNode(), idRenamingHash, view);
        outputNode.nodeProperty(nodeProperty.name()).reparentHere(newNode);
    }
}

static void syncNodeListProperties(ModelNode &outputNode, const ModelNode &inputNode, const QHash<QString, QString> &idRenamingHash, AbstractView *view)
{
    foreach (const NodeListProperty &nodeListProperty, inputNode.nodeListProperties()) {
        foreach (const ModelNode &node, nodeListProperty.toModelNodeList()) {
            ModelNode newNode = createNodeFromNode(node, idRenamingHash, view);
            outputNode.nodeListProperty(nodeListProperty.name()).reparentHere(newNode);
        }
    }
}

static ModelNode createNodeFromNode(const ModelNode &modelNode,const QHash<QString, QString> &idRenamingHash, AbstractView *view)
{
    QList<QPair<QString, QVariant> > propertyList;
    foreach (const VariantProperty &variantProperty, modelNode.variantProperties()) {
        propertyList.append(QPair<QString, QVariant>(variantProperty.name(), variantProperty.value()));
    }
    ModelNode newNode(view->createModelNode(modelNode.type(),modelNode.majorVersion(),modelNode.minorVersion(), propertyList));
    syncBindingProperties(newNode, modelNode, idRenamingHash);
    syncId(newNode, modelNode, idRenamingHash);
    syncNodeProperties(newNode, modelNode, idRenamingHash, view);
    syncNodeListProperties(newNode, modelNode, idRenamingHash, view);

    return newNode;
}

ModelNode ModelMerger::insertModel(const ModelNode &modelNode)
{
     RewriterTransaction transaction(view()->beginRewriterTransaction());

    foreach (const Import &import, modelNode.model()->imports())
        view()->model()->addImport(import);

    QHash<QString, QString> idRenamingHash;
    setupIdRenamingHash(modelNode, idRenamingHash, view());
    ModelNode newNode(createNodeFromNode(modelNode, idRenamingHash, view()));

    return newNode;
}

void ModelMerger::replaceModel(const ModelNode &modelNode)
{
     RewriterTransaction transaction(view()->beginRewriterTransaction());

    foreach (const Import &import, modelNode.model()->imports())
        view()->model()->addImport(import);
    view()->model()->setFileUrl(modelNode.model()->fileUrl());

    ModelNode rootNode(view()->rootModelNode());

    foreach (const QString &propertyName, rootNode.propertyNames())
        rootNode.removeProperty(propertyName);

    QHash<QString, QString> idRenamingHash;
    setupIdRenamingHash(modelNode, idRenamingHash, view());

    syncVariantProperties(rootNode, modelNode);
    syncBindingProperties(rootNode, modelNode, idRenamingHash);
    syncId(rootNode, modelNode, idRenamingHash);
    syncNodeProperties(rootNode, modelNode, idRenamingHash, view());
    syncNodeListProperties(rootNode, modelNode, idRenamingHash, view());
    m_view->changeRootNodeType(modelNode.type(), modelNode.majorVersion(), modelNode.minorVersion());
}

} //namespace QmlDesigner

