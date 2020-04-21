/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qmlvisualnode.h"
#include <metainfo.h>
#include "qmlchangeset.h"
#include "nodelistproperty.h"
#include "nodehints.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "qmlanchors.h"
#include "invalidmodelnodeexception.h"
#include "itemlibraryinfo.h"

#include "plaintexteditmodifier.h"
#include "rewriterview.h"
#include "modelmerger.h"
#include "rewritingexception.h"

#include <utils/qtcassert.h>

#include <QUrl>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>

namespace QmlDesigner {

bool QmlVisualNode::isItemOr3DNode(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isSubclassOf("QtQuick.Item"))
        return true;

    if (modelNode.metaInfo().isSubclassOf("QtQuick3D.Node"))
        return true;

    if (modelNode.metaInfo().isGraphicalItem() && modelNode.isRootNode())
        return true;

    return false;
}

bool QmlVisualNode::isValid() const
{
    return isValidQmlVisualNode(modelNode());
}

bool QmlVisualNode::isValidQmlVisualNode(const ModelNode &modelNode)
{
    return isValidQmlObjectNode(modelNode)
            && modelNode.metaInfo().isValid()
           && (isItemOr3DNode(modelNode) || modelNode.metaInfo().isSubclassOf("FlowView.FlowTransition")
               || modelNode.metaInfo().isSubclassOf("FlowView.FlowDecision")
               || modelNode.metaInfo().isSubclassOf("FlowView.FlowWildcard"));
}

bool QmlVisualNode::isRootNode() const
{
    return modelNode().isValid() && modelNode().isRootNode();
}

QList<QmlVisualNode> QmlVisualNode::children() const
{
    QList<ModelNode> childrenList;

    if (isValid()) {

        if (modelNode().hasNodeListProperty("children"))
                childrenList.append(modelNode().nodeListProperty("children").toModelNodeList());

        if (modelNode().hasNodeListProperty("data")) {
            for (const ModelNode &node : modelNode().nodeListProperty("data").toModelNodeList()) {
                if (QmlVisualNode::isValidQmlVisualNode(node))
                    childrenList.append(node);
            }
        }
    }

    return toQmlVisualNodeList(childrenList);
}

QList<QmlObjectNode> QmlVisualNode::resources() const
{
    QList<ModelNode> resourcesList;

    if (isValid()) {

        if (modelNode().hasNodeListProperty("resources"))
                resourcesList.append(modelNode().nodeListProperty("resources").toModelNodeList());

        if (modelNode().hasNodeListProperty("data")) {
            for (const ModelNode &node : modelNode().nodeListProperty("data").toModelNodeList()) {
                if (!QmlItemNode::isValidQmlItemNode(node))
                    resourcesList.append(node);
            }
        }
    }

    return toQmlObjectNodeList(resourcesList);
}

QList<QmlObjectNode> QmlVisualNode::allDirectSubNodes() const
{
    return toQmlObjectNodeList(modelNode().directSubModelNodes());
}

bool QmlVisualNode::hasChildren() const
{
    if (modelNode().hasNodeListProperty("children"))
        return true;

    return !children().isEmpty();
}

bool QmlVisualNode::hasResources() const
{
    if (modelNode().hasNodeListProperty("resources"))
        return true;

    return !resources().isEmpty();
}

const QList<QmlVisualNode> QmlVisualNode::allDirectSubModelNodes() const
{
    return toQmlVisualNodeList(modelNode().directSubModelNodes());
}

const QList<QmlVisualNode> QmlVisualNode::allSubModelNodes() const
{
    return toQmlVisualNodeList(modelNode().allSubModelNodes());
}

bool QmlVisualNode::hasAnySubModelNodes() const
{
    return modelNode().hasAnySubModelNodes();
}

void QmlVisualNode::setVisibilityOverride(bool visible)
{
    if (visible)
        modelNode().setAuxiliaryData("invisible", true);
    else
        modelNode().removeAuxiliaryData("invisible");
}

bool QmlVisualNode::visibilityOverride() const
{
    if (isValid())
        return modelNode().auxiliaryData("invisible").toBool();
    return false;
}

void QmlVisualNode::setDoubleProperty(const PropertyName &name, double value)
{
    modelNode().variantProperty(name).setValue(value);
}

void QmlVisualNode::initializePosition(const QmlVisualNode::Position &position)
{
    if (!position.m_2dPos.isNull()) {
        setDoubleProperty("x", qRound(position.m_2dPos.x()));
        setDoubleProperty("y", qRound(position.m_2dPos.y()));
    } else if (!position.m_3dPos.isNull()) {
        setDoubleProperty("x", position.m_3dPos.x());
        setDoubleProperty("y", position.m_3dPos.y());
        setDoubleProperty("z", position.m_3dPos.z());
    }
}

QmlModelStateGroup QmlVisualNode::states() const
{
    if (isValid())
        return QmlModelStateGroup(modelNode());
    else
        return QmlModelStateGroup();
}

QmlObjectNode QmlVisualNode::createQmlObjectNode(AbstractView *view,
                                                 const ItemLibraryEntry &itemLibraryEntry,
                                                 const Position &position,
                                                 QmlVisualNode parentQmlItemNode)
{
    if (!parentQmlItemNode.isValid())
        parentQmlItemNode = QmlVisualNode(view->rootModelNode());

    Q_ASSERT(parentQmlItemNode.isValid());

    NodeAbstractProperty parentProperty = parentQmlItemNode.defaultNodeAbstractProperty();


    NodeHints hints = NodeHints::fromItemLibraryEntry(itemLibraryEntry);
    const PropertyName forceNonDefaultProperty = hints.forceNonDefaultProperty().toUtf8();

    QmlObjectNode newNode = QmlItemNode::createQmlObjectNode(view, itemLibraryEntry, position, parentProperty);

    if (!forceNonDefaultProperty.isEmpty()) {
        if (parentQmlItemNode.modelNode().metaInfo().hasProperty(forceNonDefaultProperty))
            parentQmlItemNode.nodeListProperty(forceNonDefaultProperty).reparentHere(newNode);
    }

    return newNode;
}


static QmlObjectNode createQmlObjectNodeFromSource(AbstractView *view,
                                                   const QString &source,
                                                   const QmlVisualNode::Position &position)
{
    QScopedPointer<Model> inputModel(Model::create("QtQuick.Item", 1, 0, view->model()));
    inputModel->setFileUrl(view->model()->fileUrl());
    QPlainTextEdit textEdit;

    textEdit.setPlainText(source);
    NotIndentingTextEditModifier modifier(&textEdit);

    QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, nullptr));
    rewriterView->setCheckSemanticErrors(false);
    rewriterView->setTextModifier(&modifier);
    inputModel->setRewriterView(rewriterView.data());

    if (rewriterView->errors().isEmpty() && rewriterView->rootModelNode().isValid()) {
        ModelNode rootModelNode = rewriterView->rootModelNode();
        inputModel->detachView(rewriterView.data());
        QmlVisualNode(rootModelNode).initializePosition(position);
        ModelMerger merger(view);
        return merger.insertModel(rootModelNode);
    }

    return {};
}

QmlObjectNode QmlVisualNode::createQmlObjectNode(AbstractView *view,
                                                 const ItemLibraryEntry &itemLibraryEntry,
                                                 const Position &position,
                                                 NodeAbstractProperty parentProperty,
                                                 bool createInTransaction)
{
    QmlObjectNode newQmlObjectNode;

    auto createNodeFunc = [=, &newQmlObjectNode, &parentProperty]() {
        NodeMetaInfo metaInfo = view->model()->metaInfo(itemLibraryEntry.typeName());

        int minorVersion = metaInfo.minorVersion();
        int majorVersion = metaInfo.majorVersion();

        using PropertyBindingEntry = QPair<PropertyName, QString>;
        QList<PropertyBindingEntry> propertyBindingList;
        QList<PropertyBindingEntry> propertyEnumList;
        if (itemLibraryEntry.qmlSource().isEmpty()) {
            QList<QPair<PropertyName, QVariant> > propertyPairList = position.propertyPairList();

            for (const auto &property : itemLibraryEntry.properties()) {
                if (property.type() == "binding") {
                    propertyBindingList.append(PropertyBindingEntry(property.name(), property.value().toString()));
                } else if (property.type() == "enum")  {
                    propertyEnumList.append(PropertyBindingEntry(property.name(), property.value().toString()));
                } else {
                    propertyPairList.append({property.name(), property.value()});
                }
            }

            newQmlObjectNode = QmlObjectNode(view->createModelNode(itemLibraryEntry.typeName(), majorVersion, minorVersion, propertyPairList));
        } else {
            newQmlObjectNode = createQmlObjectNodeFromSource(view, itemLibraryEntry.qmlSource(), position);
        }

        if (parentProperty.isValid())
            parentProperty.reparentHere(newQmlObjectNode);

        if (!newQmlObjectNode.isValid())
            return;

        newQmlObjectNode.setId(view->generateNewId(itemLibraryEntry.name()));

        for (const auto &propertyBindingEntry : propertyBindingList)
            newQmlObjectNode.modelNode().bindingProperty(propertyBindingEntry.first).setExpression(propertyBindingEntry.second);

        for (const auto &propertyBindingEntry : propertyEnumList)
            newQmlObjectNode.modelNode().variantProperty(propertyBindingEntry.first).setEnumeration(propertyBindingEntry.second.toUtf8());

        Q_ASSERT(newQmlObjectNode.isValid());
    };

    if (createInTransaction)
        view->executeInTransaction("QmlItemNode::createQmlItemNode", createNodeFunc);
    else
        createNodeFunc();

    Q_ASSERT(newQmlObjectNode.isValid());

    return newQmlObjectNode;
}

QmlVisualNode QmlVisualNode::createQml3DNode(AbstractView *view,
                                             const ItemLibraryEntry &itemLibraryEntry,
                                             qint32 sceneRootId, const QVector3D &position)
{
    NodeAbstractProperty sceneNodeProperty = sceneRootId != -1 ? findSceneNodeProperty(view, sceneRootId)
                                                               : view->rootModelNode().defaultNodeAbstractProperty();

    QTC_ASSERT(sceneNodeProperty.isValid(), return {});

    return createQmlObjectNode(view, itemLibraryEntry, position, sceneNodeProperty).modelNode();
}

NodeListProperty QmlVisualNode::findSceneNodeProperty(AbstractView *view, qint32 sceneRootId)
{
    QTC_ASSERT(view, return {});

    ModelNode node;
    if (view->hasModelNodeForInternalId(sceneRootId))
        node = view->modelNodeForInternalId(sceneRootId);

    return node.defaultNodeListProperty();
}

bool QmlVisualNode::isFlowTransition(const ModelNode &node)
{
    return node.isValid()
            && node.metaInfo().isValid()
            && node.metaInfo().isSubclassOf("FlowView.FlowTransition");
}

bool QmlVisualNode::isFlowDecision(const ModelNode &node)
{
    return node.isValid()
            && node.metaInfo().isValid()
            && node.metaInfo().isSubclassOf("FlowView.FlowDecision");
}

bool QmlVisualNode::isFlowWildcard(const ModelNode &node)
{
    return node.isValid()
            && node.metaInfo().isValid()
            && node.metaInfo().isSubclassOf("FlowView.FlowWildcard");
}

bool QmlVisualNode::isFlowTransition() const
{
    return isFlowTransition(modelNode());
}

bool QmlVisualNode::isFlowDecision() const
{
    return isFlowDecision(modelNode());
}

bool QmlVisualNode::isFlowWildcard() const
{
    return isFlowWildcard(modelNode());
}

QList<ModelNode> toModelNodeList(const QList<QmlVisualNode> &qmlVisualNodeList)
{
    QList<ModelNode> modelNodeList;

    for (const QmlVisualNode &QmlVisualNode : qmlVisualNodeList)
        modelNodeList.append(QmlVisualNode.modelNode());

    return modelNodeList;
}

QList<QmlVisualNode> toQmlVisualNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<QmlVisualNode> QmlVisualNodeList;

    for (const ModelNode &modelNode : modelNodeList) {
        if (QmlVisualNode::isValidQmlVisualNode(modelNode))
            QmlVisualNodeList.append(modelNode);
    }

    return QmlVisualNodeList;
}

QStringList QmlModelStateGroup::names() const
{
    QStringList returnList;

    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode().property("states").isNodeListProperty()) {
        for (const ModelNode &node : modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState::isValidQmlModelState(node))
                returnList.append(QmlModelState(node).name());
        }
    }
    return returnList;
}

QList<QmlModelState> QmlModelStateGroup::allStates() const
{
    QList<QmlModelState> returnList;

    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode().property("states").isNodeListProperty()) {
        for (const ModelNode &node : modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState::isValidQmlModelState(node))
                returnList.append(node);
        }
    }
    return returnList;
}

QmlModelState QmlModelStateGroup::addState(const QString &name)
{
    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    ModelNode newState = QmlModelState::createQmlState(
                modelNode().view(), {{PropertyName("name"), QVariant(name)}});
    modelNode().nodeListProperty("states").reparentHere(newState);

    return newState;
}

void QmlModelStateGroup::removeState(const QString &name)
{
    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (state(name).isValid())
        state(name).modelNode().destroy();
}

QmlModelState QmlModelStateGroup::state(const QString &name) const
{
    if (!modelNode().isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode().property("states").isNodeListProperty()) {
        for (const ModelNode &node : modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState(node).name() == name)
                return node;
        }
    }
    return QmlModelState();
}

QList<QPair<PropertyName, QVariant> > QmlVisualNode::Position::propertyPairList() const
{
    QList<QPair<PropertyName, QVariant> > propertyPairList;

    if (!m_2dPos.isNull()) {
        propertyPairList.append({"x", QVariant(qRound(m_2dPos.x()))});
        propertyPairList.append({"y", QVariant(qRound(m_2dPos.y()))});
    } else if (!m_3dPos.isNull()) {
        propertyPairList.append({"x", QVariant(m_3dPos.x())});
        propertyPairList.append({"y", QVariant(m_3dPos.y())});
        propertyPairList.append({"z", QVariant(m_3dPos.z())});
    }

    return propertyPairList;
}

} //QmlDesigner
