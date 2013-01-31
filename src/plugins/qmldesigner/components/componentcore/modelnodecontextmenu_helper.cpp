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

#include "modelnodecontextmenu_helper.h"

#include <nodemetainfo.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <bindingproperty.h>
#include <nodeproperty.h>
#include <designmodewidget.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

static inline DesignDocument* currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
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

    if (node.metaInfo().isFileComponent())
        return true;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource)
        return true;
    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isFileComponent())
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

    if (node.metaInfo().isFileComponent())
        return true;

    if (checkIfNodeIsAView(node) &&
        node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().metaInfo().isFileComponent())
            return true;
    }

    return false;
}

static inline void openFileForComponent(const ModelNode &node)
{
    QmlDesignerPlugin::instance()->viewManager().nextFileIsCalledInternally();

    //int width = 0;
    //int height = 0;
    QHash<QString, QVariant> propertyHash;
    if (node.metaInfo().isFileComponent()) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        currentDesignDocument()->changeToExternalSubComponent(node.metaInfo().componentFileName());
    } else if (checkIfNodeIsAView(node) &&
               node.hasNodeProperty("delegate") &&
               node.nodeProperty("delegate").modelNode().metaInfo().isFileComponent()) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        currentDesignDocument()->changeToExternalSubComponent(node.nodeProperty("delegate").modelNode().metaInfo().componentFileName());
    }
    ModelNode rootModelNode = currentDesignDocument()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
    //rootModelNode.setAuxiliaryData("width", width);
    //rootModelNode.setAuxiliaryData("height", height);
}

static inline void openInlineComponent(const ModelNode &node)
{

    if (!node.isValid() || !node.metaInfo().isValid())
        return;

    if (!currentDesignDocument())
        return;

    QHash<QString, QVariant> propertyHash;

    if (node.nodeSourceType() == ModelNode::NodeWithComponentSource) {
        //getWidthHeight(node, width, height);
        getProperties(node, propertyHash);
        currentDesignDocument()->changeToSubComponent(node);
    } else if (checkIfNodeIsAView(node) &&
               node.hasNodeProperty("delegate")) {
        if (node.nodeProperty("delegate").modelNode().nodeSourceType() == ModelNode::NodeWithComponentSource) {
            //getWidthHeight(node, width, height);
            getProperties(node, propertyHash);
            currentDesignDocument()->changeToSubComponent(node.nodeProperty("delegate").modelNode());
        }
    }

    ModelNode rootModelNode = currentDesignDocument()->rewriterView()->rootModelNode();
    applyProperties(rootModelNode, propertyHash);
    //rootModelNode.setAuxiliaryData("width", width);
    //rootModelNode.setAuxiliaryData("height", height);
}

namespace ComponentUtils {
void goIntoComponent(const ModelNode &modelNode)
{
    if (modelNode.isValid() && modelNodeIsComponent(modelNode)) {
        QmlDesignerPlugin::instance()->viewManager().setComponentNode(modelNode);
        if (isFileComponent(modelNode))
            openFileForComponent(modelNode);
        else
            openInlineComponent(modelNode);
    }
}

} // namespace ComponentUtils

namespace SelectionContextFunctors {

bool singleSelectionItemIsAnchored(const SelectionContext &selectionState)
{
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool anchored = itemNode.instanceHasAnchors();
        return anchored;
    }
    return false;
}

bool singleSelectionItemIsNotAnchored(const SelectionContext &selectionState)
{
    QmlItemNode itemNode(selectionState.currentSingleSelectedNode());
    if (selectionState.isInBaseState() && itemNode.isValid()) {
        bool anchored = itemNode.instanceHasAnchors();
        return !anchored;
    }
    return false;
}

bool selectionHasSameParent(const SelectionContext &selectionState)
{
    return !selectionState.selectedModelNodes().isEmpty() && itemsHaveSameParent(selectionState.selectedModelNodes());
}

bool selectionIsComponent(const SelectionContext &selectionState)
{
    return modelNodeIsComponent(selectionState.currentSingleSelectedNode());
}

} //SelectionStateFunctors

} //QmlDesigner
