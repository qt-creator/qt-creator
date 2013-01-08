/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "modelnodecontextmenu_helper.h"

#include <nodemetainfo.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <bindingproperty.h>
#include <nodeproperty.h>
#include <designmodewidget.h>

namespace QmlDesigner {

static inline DesignDocumentController* designDocumentController()
{
    return Internal::DesignModeWidget::instance()->currentDesignDocumentController();
}

static inline bool checkIfNodeIsAView(const ModelNode &node)
{
    return node.metaInfo().isValid() &&
            (node.metaInfo().isSubclassOf("QtQuick.ListView", -1, -1) ||
             node.metaInfo().isSubclassOf("QtQuick.GridView", -1, -1) ||
             node.metaInfo().isSubclassOf("QtQuick.PathView", -1, -1));
}

static inline void getProperties(const ModelNode node, QHash<QString, QVariant> &propertyHash)
{
    if (QmlObjectNode(node).isValid()) {
        foreach (const QString &propertyName, node.propertyNames()) {
            if (node.property(propertyName).isVariantProperty() ||
                    (node.property(propertyName).isBindingProperty() &&
                     !propertyName.contains(QLatin1String("anchors.")))) {
                propertyHash.insert(propertyName, QmlObjectNode(node).instanceValue(propertyName));
            }
        }
    }
    QmlItemNode itemNode(node);
    if (itemNode.isValid()) {
        propertyHash.insert(QLatin1String("width"), itemNode.instanceValue(QLatin1String("width")));
        propertyHash.insert(QLatin1String("height"), itemNode.instanceValue(QLatin1String("height")));
        propertyHash.remove(QLatin1String("x"));
        propertyHash.remove(QLatin1String("y"));
        propertyHash.remove(QLatin1String("rotation"));
        propertyHash.remove(QLatin1String("opacity"));
    }
}

static inline void applyProperties(ModelNode &node, const QHash<QString, QVariant> &propertyHash)
{
    QHash<QString, QVariant> auxiliaryData  = node.auxiliaryData();
    foreach (const QString propertyName, auxiliaryData.keys()) {
        node.setAuxiliaryData(propertyName, QVariant());
    }

    QHashIterator<QString, QVariant> propertyIterator(propertyHash);
    while (propertyIterator.hasNext()) {
        propertyIterator.next();
        const QString propertyName = propertyIterator.key();
        if (propertyName == QLatin1String("width") || propertyName == QLatin1String("height")) {
            node.setAuxiliaryData(propertyIterator.key(), propertyIterator.value());
        } else if (node.property(propertyIterator.key()).isDynamic() &&
                   node.property(propertyIterator.key()).dynamicTypeName() == QLatin1String("alias") &&
                   node.property(propertyIterator.key()).isBindingProperty()) {
            AbstractProperty targetProperty = node.bindingProperty(propertyIterator.key()).resolveToProperty();
            if (targetProperty.isValid())
                targetProperty.parentModelNode().setAuxiliaryData(targetProperty.name() + QLatin1String("@NodeInstance"), propertyIterator.value());
        } else {
            node.setAuxiliaryData(propertyIterator.key() + QLatin1String("@NodeInstance"), propertyIterator.value());
        }
    }
}

static inline bool modelNodeIsComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return false;

    if (node.metaInfo().isComponent())
        return true;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource)
        return true;
    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isComponent())
            return true;
        if (node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource)
            return true;
    }

    return false;
}

static inline bool itemsHaveSameParent(const QList<ModelNode> &siblingList)
{
    if (siblingList.isEmpty())
        return false;


    QmlItemNode item(siblingList.first());
    if (!item.isValid())
        return false;

    if (item.isRootModelNode())
        return false;

    QmlItemNode parent = item.instanceParent().toQmlItemNode();
    if (!parent.isValid())
        return false;

    foreach (const ModelNode &node, siblingList) {
        QmlItemNode currentItem(node);
        if (!currentItem.isValid())
            return false;
        QmlItemNode currentParent = currentItem.instanceParent().toQmlItemNode();
        if (!currentParent.isValid())
            return false;
        if (currentItem.instanceIsInPositioner())
            return false;
        if (currentParent != parent)
            return false;
    }
    return true;
}

static inline bool isFileComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return false;

    if (node.metaInfo().isComponent())
        return true;

    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isComponent())
            return true;
    }

    return false;
}

static inline void openFileForComponent(const ModelNode &node)
{
    //int width = 0;
    //int height = 0;
    QHash<QString, QVariant> propertyHash;
    if (node.metaInfo().isComponent()) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        designDocumentController()->changeToExternalSubComponent(node.metaInfo().componentFileName());
    } else if (checkIfNodeIsAView(node) &&
               node.hasNodeProperty("delegate") &&
               node.nodeProperty("delegate").modelNode().metaInfo().isComponent()) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        designDocumentController()->changeToExternalSubComponent(node.nodeProperty("delegate").modelNode().metaInfo().componentFileName());
    }
    ModelNode rootModelNode = designDocumentController()->model()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
    //rootModelNode.setAuxiliaryData("width", width);
    //rootModelNode.setAuxiliaryData("height", height);
}

static inline void openInlineComponent(const ModelNode &node)
{
    if (!node.isValid() || !node.metaInfo().isValid())
        return;

    if (!designDocumentController())
        return;

    QHash<QString, QVariant> propertyHash;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        designDocumentController()->changeToSubComponent(node);
    } else if (checkIfNodeIsAView(node) &&
               node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource) {
            //getWidthHeight(node, width, height);
            getProperties(node, propertyHash);
            designDocumentController()->changeToSubComponent(node.nodeProperty("delegate").modelNode());
        }
    }

    ModelNode rootModelNode = designDocumentController()->model()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
    //rootModelNode.setAuxiliaryData("width", width);
    //rootModelNode.setAuxiliaryData("height", height);
}

void ComponentUtils::goIntoComponent(const ModelNode &modelNode)
{

    if (modelNode.isValid() && modelNodeIsComponent(modelNode)) {
        if (isFileComponent(modelNode))
            openFileForComponent(modelNode);
        else
            openInlineComponent(modelNode);
    }
}

namespace SelectionContextFunctors {

bool SingleSelectionItemIsAnchored::operator() (const SelectionContext &selectionState) {
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool anchored = itemNode.instanceHasAnchors();
        return anchored;
    }
    return false;
}

bool SingleSelectionItemNotAnchored::operator() (const SelectionContext &selectionState) {
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool anchored = itemNode.instanceHasAnchors();
        return !anchored;
    }
    return false;
}

bool SelectionHasSameParent::operator() (const SelectionContext &selectionState)
{
    return !selectionState.selectedModelNodes().isEmpty() && itemsHaveSameParent(selectionState.selectedModelNodes());
}

bool SelectionIsComponent::operator() (const SelectionContext &selectionState)
{
    return modelNodeIsComponent(selectionState.currentSingleSelectedNode());
}

} //SelectionStateFunctors

} //QmlDesigner
