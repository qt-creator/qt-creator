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

#include "modelmerger.h"

#include "modelnode.h"
#include "abstractview.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "nodelistproperty.h"
#include "bindingproperty.h"
#include "variantproperty.h"
#include "rewritertransaction.h"
#include <rewritingexception.h>

#include <QUrl>

#include <QDebug>

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
        outputNode.variantProperty(variantProperty.name()).setValue(variantProperty.value());
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
    if (!inputNode.id().isEmpty())
        outputNode.setIdWithoutRefactoring(idRenamingHash.value(inputNode.id()));
}

static void splitIdInBaseNameAndNumber(const QString &id, QString *baseId, int *number)
{

    int counter = 0;
    while (counter < id.count()) {
        bool canConvertToInteger = false;
        int newNumber = id.rightRef(counter +1).toInt(&canConvertToInteger);
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
    foreach (const ModelNode &node, modelNode.allSubModelNodesAndThisNode()) {
        if (!node.id().isEmpty()) {
            QString newId = node.id();
            QString baseId;
            int number = 1;
            splitIdInBaseNameAndNumber(newId, &baseId, &number);

            while (view->hasId(newId) || idRenamingHash.values().contains(newId)) {
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
    QList<QPair<PropertyName, QVariant> > propertyList;
    QList<QPair<PropertyName, QVariant> > variantPropertyList;
    foreach (const VariantProperty &variantProperty, modelNode.variantProperties()) {
        propertyList.append(QPair<PropertyName, QVariant>(variantProperty.name(), variantProperty.value()));
    }
    NodeMetaInfo nodeMetaInfo = view->model()->metaInfo(modelNode.type());
    ModelNode newNode(view->createModelNode(modelNode.type(), nodeMetaInfo.majorVersion(), nodeMetaInfo.minorVersion(),
                                            propertyList, variantPropertyList, modelNode.nodeSource(), modelNode.nodeSourceType()));
    syncBindingProperties(newNode, modelNode, idRenamingHash);
    syncId(newNode, modelNode, idRenamingHash);
    syncNodeProperties(newNode, modelNode, idRenamingHash, view);
    syncNodeListProperties(newNode, modelNode, idRenamingHash, view);

    return newNode;
}

ModelNode ModelMerger::insertModel(const ModelNode &modelNode)
{
    RewriterTransaction transaction(view()->beginRewriterTransaction(QByteArrayLiteral("ModelMerger::insertModel")));

    QList<Import> newImports;

    foreach (const Import &import, modelNode.model()->imports()) {
        if (!view()->model()->hasImport(import, true, true))
            newImports.append(import);
    }

    view()->model()->changeImports(newImports, {});

    QHash<QString, QString> idRenamingHash;
    setupIdRenamingHash(modelNode, idRenamingHash, view());
    ModelNode newNode(createNodeFromNode(modelNode, idRenamingHash, view()));

    return newNode;
}

void ModelMerger::replaceModel(const ModelNode &modelNode)
{
        view()->model()->changeImports(modelNode.model()->imports(), {});
        view()->model()->setFileUrl(modelNode.model()->fileUrl());

    try {
        RewriterTransaction transaction(view()->beginRewriterTransaction(QByteArrayLiteral("ModelMerger::replaceModel")));

        ModelNode rootNode(view()->rootModelNode());

        foreach (const PropertyName &propertyName, rootNode.propertyNames())
            rootNode.removeProperty(propertyName);

        QHash<QString, QString> idRenamingHash;
        setupIdRenamingHash(modelNode, idRenamingHash, view());

        syncVariantProperties(rootNode, modelNode);
        syncBindingProperties(rootNode, modelNode, idRenamingHash);
        syncId(rootNode, modelNode, idRenamingHash);
        syncNodeProperties(rootNode, modelNode, idRenamingHash, view());
        syncNodeListProperties(rootNode, modelNode, idRenamingHash, view());
        m_view->changeRootNodeType(modelNode.type(), modelNode.majorVersion(), modelNode.minorVersion());

        transaction.commit();
    } catch (const RewritingException &e) {
        qWarning() << e.description(); //silent error
    }
}

} //namespace QmlDesigner

