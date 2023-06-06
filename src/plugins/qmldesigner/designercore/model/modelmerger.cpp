// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelmerger.h"

#include "modelnode.h"
#include "abstractview.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "nodelistproperty.h"
#include "bindingproperty.h"
#include "variantproperty.h"
#include "rewritertransaction.h"
#include "signalhandlerproperty.h"
#include <rewritingexception.h>

#include <QUrl>

#include <QDebug>
#include <QtCore/qregularexpression.h>

namespace QmlDesigner {

static ModelNode createNodeFromNode(const ModelNode &modelNode,
                                    const QHash<QString, QString> &idRenamingHash,
                                    AbstractView *view, const MergePredicate &mergePredicate);

static QString fixExpression(const QString &expression, const QHash<QString, QString> &idRenamingHash)
{
    const QString pattern("\\b%1\\b"); // Match only full ids
    QString newExpression = expression;
    const auto keys = idRenamingHash.keys();
    for (const QString &id : keys) {
        QRegularExpression re(pattern.arg(id));
        if (newExpression.contains(re))
            newExpression = newExpression.replace(re, idRenamingHash.value(id));
    }
    return newExpression;
}

static void syncVariantProperties(ModelNode &outputNode, const ModelNode &inputNode)
{
    const QList<VariantProperty> propertyies = inputNode.variantProperties();
    for (const VariantProperty &property : propertyies) {
        if (property.isDynamic()) {
            outputNode.variantProperty(property.name()).
                    setDynamicTypeNameAndValue(property.dynamicTypeName(), property.value());
            continue;
        }
        outputNode.variantProperty(property.name()).setValue(property.value());
    }
}

static void syncAuxiliaryProperties(ModelNode &outputNode, const ModelNode &inputNode)
{
    for (const auto &element : inputNode.auxiliaryData())
        outputNode.setAuxiliaryData(AuxiliaryDataKeyView{element.first}, element.second);
}

static void syncBindingProperties(ModelNode &outputNode, const ModelNode &inputNode, const QHash<QString, QString> &idRenamingHash)
{
    const QList<BindingProperty> propertyies = inputNode.bindingProperties();
    for (const BindingProperty &bindingProperty : propertyies) {
        outputNode.bindingProperty(bindingProperty.name()).setExpression(fixExpression(bindingProperty.expression(), idRenamingHash));
    }
}

static void syncSignalHandlerProperties(ModelNode &outputNode, const ModelNode &inputNode,  const QHash<QString, QString> &idRenamingHash)
{
    const QList<SignalHandlerProperty> propertyies = inputNode.signalProperties();
    for (const SignalHandlerProperty &signalProperty : propertyies) {
        outputNode.signalHandlerProperty(signalProperty.name()).setSource(fixExpression(signalProperty.source(), idRenamingHash));
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
    while (counter < id.size()) {
        bool canConvertToInteger = false;
        int newNumber = id.right(counter + 1).toInt(&canConvertToInteger);
        if (canConvertToInteger)
            *number = newNumber;
        else
            break;

        counter++;
    }

    *baseId = id.left(id.size() - counter);
}

static void setupIdRenamingHash(const ModelNode &modelNode, QHash<QString, QString> &idRenamingHash, AbstractView *view)
{
    const QList<ModelNode> nodes = modelNode.allSubModelNodesAndThisNode();
    for (const ModelNode &node : nodes) {
        if (!node.id().isEmpty()) {
            QString newId = node.id();
            QString baseId;
            int number = 1;
            splitIdInBaseNameAndNumber(newId, &baseId, &number);

            while (view->hasId(newId) || std::find(idRenamingHash.cbegin(),
                        idRenamingHash.cend(), newId) != idRenamingHash.cend()) {
                newId = baseId + QString::number(number);
                number++;
            }

            idRenamingHash.insert(node.id(), newId);
        }
    }
}

static void syncNodeProperties(ModelNode &outputNode, const ModelNode &inputNode,
                               const QHash<QString, QString> &idRenamingHash,
                               AbstractView *view, const MergePredicate &mergePredicate)
{
    const QList<NodeProperty> propertyies = inputNode.nodeProperties();
    for (const NodeProperty &nodeProperty : propertyies) {
        ModelNode node = nodeProperty.modelNode();
        if (!mergePredicate(node))
            continue;
        ModelNode newNode = createNodeFromNode(node, idRenamingHash, view, mergePredicate);
        outputNode.nodeProperty(nodeProperty.name()).reparentHere(newNode);
    }
}

static void syncNodeListProperties(ModelNode &outputNode, const ModelNode &inputNode,
                                   const QHash<QString, QString> &idRenamingHash,
                                   AbstractView *view, const MergePredicate &mergePredicate)
{
    const QList<NodeListProperty> propertyies = inputNode.nodeListProperties();
    for (const NodeListProperty &nodeListProperty : propertyies) {
        const QList<ModelNode> nodes = nodeListProperty.toModelNodeList();
        for (const ModelNode &node : nodes) {
            if (!mergePredicate(node))
                continue;
            ModelNode newNode = createNodeFromNode(node, idRenamingHash, view, mergePredicate);
            outputNode.nodeListProperty(nodeListProperty.name()).reparentHere(newNode);
        }
    }
}

static ModelNode createNodeFromNode(const ModelNode &modelNode,
                                    const QHash<QString, QString> &idRenamingHash,
                                    AbstractView *view, const MergePredicate &mergePredicate)
{
    NodeMetaInfo nodeMetaInfo = view->model()->metaInfo(modelNode.type());
    ModelNode newNode(view->createModelNode(modelNode.type(), nodeMetaInfo.majorVersion(), nodeMetaInfo.minorVersion(),
                                            {}, {}, modelNode.nodeSource(), modelNode.nodeSourceType()));
    syncVariantProperties(newNode, modelNode);
    syncAuxiliaryProperties(newNode, modelNode);
    syncBindingProperties(newNode, modelNode, idRenamingHash);
    syncSignalHandlerProperties(newNode, modelNode, idRenamingHash);
    syncId(newNode, modelNode, idRenamingHash);
    syncNodeProperties(newNode, modelNode, idRenamingHash, view, mergePredicate);
    syncNodeListProperties(newNode, modelNode, idRenamingHash, view, mergePredicate);

    return newNode;
}


ModelNode ModelMerger::insertModel(const ModelNode &modelNode, const MergePredicate &predicate)
{
    if (!predicate(modelNode))
        return {};
    RewriterTransaction transaction(view()->beginRewriterTransaction(QByteArrayLiteral("ModelMerger::insertModel")));

    Imports newImports;

    for (const Import &import : modelNode.model()->imports()) {
        if (!view()->model()->hasImport(import, true, true))
            newImports.append(import);
    }

    view()->model()->changeImports(newImports, {});

    QHash<QString, QString> idRenamingHash;
    setupIdRenamingHash(modelNode, idRenamingHash, view());
    ModelNode newNode(createNodeFromNode(modelNode, idRenamingHash, view(), predicate));

    return newNode;
}

void ModelMerger::replaceModel(const ModelNode &modelNode, const MergePredicate &predicate)
{
    if (!predicate(modelNode))
        return;

    view()->model()->changeImports(modelNode.model()->imports(), {});
    view()->model()->setFileUrl(modelNode.model()->fileUrl());

    view()->executeInTransaction("ModelMerger::replaceModel", [this, modelNode, &predicate](){
        ModelNode rootNode(view()->rootModelNode());

        const QList<PropertyName> propertyNames = rootNode.propertyNames();
        for (const PropertyName &propertyName : propertyNames)
            rootNode.removeProperty(propertyName);

        QHash<QString, QString> idRenamingHash;
        setupIdRenamingHash(modelNode, idRenamingHash, view());

        syncAuxiliaryProperties(rootNode, modelNode);
        syncVariantProperties(rootNode, modelNode);
        syncBindingProperties(rootNode, modelNode, idRenamingHash);
        syncId(rootNode, modelNode, idRenamingHash);
        syncNodeProperties(rootNode, modelNode, idRenamingHash, view(), predicate);
        syncNodeListProperties(rootNode, modelNode, idRenamingHash, view(), predicate);
        m_view->changeRootNodeType(modelNode.type(), modelNode.majorVersion(), modelNode.minorVersion());
    });
}

} //namespace QmlDesigner

