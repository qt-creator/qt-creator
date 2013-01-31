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

#include "modelmerger.h"

#include "modelmerger.h"
#include "modelnode.h"
#include "abstractview.h"
#include "model.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "nodelistproperty.h"
#include "bindingproperty.h"
#include "variantproperty.h"
#include "rewritertransaction.h"
#include <rewritingexception.h>

#include <QSet>
#include <QStringList>
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
    if (!inputNode.id().isEmpty())
        outputNode.setId(idRenamingHash.value(inputNode.id()));
}

static void splitIdInBaseNameAndNumber(const QString &id, QString *baseId, int *number)
{

    int counter = 0;
    while (counter < id.count()) {
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
    QList<QPair<QString, QVariant> > propertyList;
    QList<QPair<QString, QVariant> > variantPropertyList;
    foreach (const VariantProperty &variantProperty, modelNode.variantProperties()) {
        propertyList.append(QPair<QString, QVariant>(variantProperty.name(), variantProperty.value()));
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
    RewriterTransaction transaction(view()->beginRewriterTransaction());

    QList<Import> newImports;

    foreach (const Import &import, modelNode.model()->imports()) {
        if (!view()->model()->hasImport(import, true, true))
            newImports.append(import);
    }

    view()->model()->changeImports(newImports, QList<Import>());

    QHash<QString, QString> idRenamingHash;
    setupIdRenamingHash(modelNode, idRenamingHash, view());
    qDebug() << idRenamingHash;
    ModelNode newNode(createNodeFromNode(modelNode, idRenamingHash, view()));

    return newNode;
}

void ModelMerger::replaceModel(const ModelNode &modelNode)
{
        view()->model()->changeImports(modelNode.model()->imports(), QList<Import>());
        view()->model()->setFileUrl(modelNode.model()->fileUrl());

    try {
        RewriterTransaction transaction(view()->beginRewriterTransaction());

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
    } catch (RewritingException &e) {
        qWarning() << e.description(); //silent error
    }
}

} //namespace QmlDesigner

