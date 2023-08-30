// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlvisualnode.h"
#include <metainfo.h>
#include "qmlchangeset.h"
#include "nodelistproperty.h"
#include "nodehints.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "qmlanchors.h"
#include "itemlibraryinfo.h"

#include <externaldependenciesinterface.h>

#include "plaintexteditmodifier.h"
#include "rewriterview.h"
#include "modelmerger.h"

#include <utils/qtcassert.h>

#include <QUrl>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>
#include <QRandomGenerator>

namespace QmlDesigner {

static char imagePlaceHolder[] = "qrc:/qtquickplugin/images/template_image.png";

bool QmlVisualNode::isItemOr3DNode(const ModelNode &modelNode)
{
    auto metaInfo = modelNode.metaInfo();
    auto model = modelNode.model();

    if (metaInfo.isBasedOn(model->qtQuickItemMetaInfo(), model->qtQuick3DNodeMetaInfo()))
        return true;

    if (metaInfo.isGraphicalItem() && modelNode.isRootNode())
        return true;

    return false;
}

bool QmlVisualNode::isValid() const
{
    return isValidQmlVisualNode(modelNode());
}

bool QmlVisualNode::isValidQmlVisualNode(const ModelNode &modelNode)
{
    if (!isValidQmlObjectNode(modelNode))
        return false;

    auto metaInfo = modelNode.metaInfo();
    auto model = modelNode.model();

    return isItemOr3DNode(modelNode) || metaInfo.isBasedOn(model->flowViewFlowTransitionMetaInfo(),
                              model->flowViewFlowDecisionMetaInfo(),
                              model->flowViewFlowWildcardMetaInfo());
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
        modelNode().setAuxiliaryData(invisibleProperty, true);
    else
        modelNode().removeAuxiliaryData(invisibleProperty);
}

bool QmlVisualNode::visibilityOverride() const
{
    if (isValid())
        return modelNode().auxiliaryDataWithDefault(invisibleProperty).toBool();
    return false;
}

void QmlVisualNode::scatter(const ModelNode &targetNode, const std::optional<int> &offset)
{
    if (!isValid())
        return;

    if (targetNode.metaInfo().isValid() && targetNode.metaInfo().isLayoutable())
        return;

    bool scatter = false;
    const QList<ModelNode> targetDirectNodes = targetNode.directSubModelNodes();
    for (const ModelNode &childNode : targetDirectNodes) {
        if (childNode == modelNode())
            continue;

        if (isValidQmlVisualNode(childNode)) {
            Position childPos = QmlVisualNode(childNode).position();
            if (qFuzzyCompare(position().distanceToPoint(childPos), 0.f)) {
                scatter = true;
                break;
            }
        }
    }

    if (!scatter)
        return;

    if (offset.has_value()) { // offset
        double offsetValue = offset.value();
        this->translate(QVector3D(offsetValue, offsetValue, offsetValue));
    } else { // scatter in range
        const double scatterRange = 20.;
        double x = QRandomGenerator::global()->generateDouble() * scatterRange - scatterRange / 2;
        double y = QRandomGenerator::global()->generateDouble() * scatterRange - scatterRange / 2;
        double z = (modelNode().metaInfo().isQtQuick3DNode())
                ? QRandomGenerator::global()->generateDouble() * scatterRange - scatterRange / 2
                : 0.;
        this->translate(QVector3D(x, y, z));
    }
}

void QmlVisualNode::translate(const QVector3D &vector)
{
    setPosition(position() + vector);
}

void QmlVisualNode::setDoubleProperty(const PropertyName &name, double value)
{
    modelNode().variantProperty(name).setValue(value);
}

void QmlVisualNode::setPosition(const QmlVisualNode::Position &position)
{
    if (!modelNode().isValid())
        return;

    if (!qFuzzyIsNull(position.x()) || modelNode().hasProperty("x"))
        setDoubleProperty("x", position.x());
    if (!qFuzzyIsNull(position.y()) || modelNode().hasProperty("y"))
        setDoubleProperty("y", position.y());

    if (position.is3D()
            && (!qFuzzyIsNull(position.z()) || modelNode().hasProperty("z"))
            && modelNode().metaInfo().isQtQuick3DNode()) {
        setDoubleProperty("z", position.z());
    }
}

QmlVisualNode::Position QmlVisualNode::position() const
{
    if (!isValid())
        return {};

    double x = modelNode().variantProperty("x").value().toDouble();
    double y = modelNode().variantProperty("y").value().toDouble();

    if (modelNode().metaInfo().isQtQuick3DModel()) {
        double z = modelNode().variantProperty("z").value().toDouble();
        return Position(QVector3D(x,y,z));
    }
    return Position(QPointF(x,y));
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

    QmlObjectNode newNode = QmlItemNode::createQmlObjectNode(view,
                                                             itemLibraryEntry,
                                                             position,
                                                             parentProperty);

    if (!forceNonDefaultProperty.isEmpty()) {
        const NodeMetaInfo metaInfo = parentQmlItemNode.modelNode().metaInfo();
        if (metaInfo.hasProperty(forceNonDefaultProperty)) {
            if (!metaInfo.property(forceNonDefaultProperty).isListProperty()
                && parentQmlItemNode.modelNode().hasNodeProperty(forceNonDefaultProperty)) {
                parentQmlItemNode.removeProperty(forceNonDefaultProperty);
            }
            parentQmlItemNode.nodeListProperty(forceNonDefaultProperty).reparentHere(newNode);
        }
    }

    return newNode;
}


static QmlObjectNode createQmlObjectNodeFromSource(AbstractView *view,
                                                   const QString &source,
                                                   const QmlVisualNode::Position &position)
{
    auto inputModel = Model::create("QtQuick.Item", 1, 0, view->model());
    inputModel->setFileUrl(view->model()->fileUrl());
    QPlainTextEdit textEdit;

    textEdit.setPlainText(source);
    NotIndentingTextEditModifier modifier(&textEdit);

    QScopedPointer<RewriterView> rewriterView(
        new RewriterView(view->externalDependencies(), RewriterView::Amend));
    rewriterView->setCheckSemanticErrors(false);
    rewriterView->setTextModifier(&modifier);
    rewriterView->setAllowComponentRoot(true);
    rewriterView->setPossibleImportsEnabled(false);
    inputModel->setRewriterView(rewriterView.data());

    if (rewriterView->errors().isEmpty() && rewriterView->rootModelNode().isValid()) {
        ModelNode rootModelNode = rewriterView->rootModelNode();
        inputModel->detachView(rewriterView.data());
        QmlVisualNode(rootModelNode).setPosition(position);
        ModelMerger merger(view);
        return merger.insertModel(rootModelNode);
    }

    return {};
}

static QString imagePlaceHolderPath(AbstractView *view)
{
    QFileInfo info(view->externalDependencies().projectUrl().toLocalFile() + "/images/place_holder.png");

    if (info.exists()) {
        const QDir dir(QFileInfo(view->model()->fileUrl().toLocalFile()).absoluteDir());
        return dir.relativeFilePath(info.filePath());
    }


    return QString::fromLatin1(imagePlaceHolder);
}

QmlObjectNode QmlVisualNode::createQmlObjectNode(AbstractView *view,
                                                 const ItemLibraryEntry &itemLibraryEntry,
                                                 const Position &position,
                                                 NodeAbstractProperty parentProperty,
                                                 bool createInTransaction)
{
    QmlObjectNode newQmlObjectNode;

    NodeHints hints = NodeHints::fromItemLibraryEntry(itemLibraryEntry);

    auto createNodeFunc = [=, &newQmlObjectNode, &parentProperty]() {
        NodeMetaInfo metaInfo = view->model()->metaInfo(itemLibraryEntry.typeName());

        int minorVersion = metaInfo.minorVersion();
        int majorVersion = metaInfo.majorVersion();

        using PropertyBindingEntry = QPair<PropertyName, QString>;
        QList<PropertyBindingEntry> propertyBindingList;
        QList<PropertyBindingEntry> propertyEnumList;
        if (itemLibraryEntry.qmlSource().isEmpty()) {
            QList<QPair<PropertyName, QVariant> > propertyPairList;

            for (const auto &property : itemLibraryEntry.properties()) {
                if (property.type() == "binding") {
                    const QString value = QmlObjectNode::convertToCorrectTranslatableFunction(
                        property.value().toString(), view->externalDependencies().designerSettings());
                    propertyBindingList.append(PropertyBindingEntry(property.name(), value));
                } else if (property.type() == "enum") {
                    propertyEnumList.append(PropertyBindingEntry(property.name(), property.value().toString()));
                } else if (property.value().toString() == QString::fromLatin1(imagePlaceHolder)) {
                    propertyPairList.append({property.name(), imagePlaceHolderPath(view)});
                } else {
                    propertyPairList.append({property.name(), property.value()});
                }
            }
            // Add position last so it'll override any default position specified in the entry
            propertyPairList.append(position.propertyPairList());

            ModelNode::NodeSourceType nodeSourceType = ModelNode::NodeWithoutSource;
            if (itemLibraryEntry.typeName() == "QtQml.Component")
                nodeSourceType = ModelNode::NodeWithComponentSource;

            newQmlObjectNode = QmlObjectNode(view->createModelNode(itemLibraryEntry.typeName(), majorVersion, minorVersion, propertyPairList, {}, {}, nodeSourceType));
        } else {
            newQmlObjectNode = createQmlObjectNodeFromSource(view, itemLibraryEntry.qmlSource(), position);
        }

        if (parentProperty.isValid()) {
            const PropertyName propertyName = parentProperty.name();
            const ModelNode parentNode = parentProperty.parentModelNode();
            const NodeMetaInfo metaInfo = parentNode.metaInfo();

            if (metaInfo.isValid() && !metaInfo.property(propertyName).isListProperty()
                && parentProperty.isNodeProperty()) {
                parentNode.removeProperty(propertyName);
            }

            parentNode.nodeAbstractProperty(propertyName).reparentHere(newQmlObjectNode);
        }

        if (!newQmlObjectNode.isValid())
            return;

        if (newQmlObjectNode.id().isEmpty())
            newQmlObjectNode.modelNode().setIdWithoutRefactoring(view->model()->generateNewId(itemLibraryEntry.name()));

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

    if (!hints.setParentProperty().first.isEmpty() && parentProperty.isValid()) {
        ModelNode parent = parentProperty.parentModelNode();
        const PropertyName property = hints.setParentProperty().first.toUtf8();
        const QVariant value = hints.setParentProperty().second;

        parent.variantProperty(property).setValue(value);
    }

    if (!hints.bindParentToProperty().isEmpty() && parentProperty.isValid()) {
        const PropertyName property = hints.bindParentToProperty().toUtf8();
        ModelNode parent = parentProperty.parentModelNode();

        const NodeMetaInfo metaInfo = newQmlObjectNode.modelNode().metaInfo();
        if (metaInfo.hasProperty(property))
            newQmlObjectNode.setBindingProperty(property, parent.validId());
    }

    const QStringList copyFiles = itemLibraryEntry.extraFilePaths();
    if (!copyFiles.isEmpty()) {
        // Files are copied into the same directory as the current qml document
        for (const auto &copyFileStr : copyFiles) {
            Utils::FilePath sourceFile = Utils::FilePath::fromString(copyFileStr);
            Utils::FilePath qmlFilePath = Utils::FilePath::fromString(
                                            view->model()->fileUrl().toLocalFile()).absolutePath();
            Utils::FilePath targetFile = qmlFilePath.pathAppended(sourceFile.fileName());
            // We don't want to overwrite existing default files
            if (!targetFile.exists() && !sourceFile.copyFile(targetFile))
                qWarning() << QStringView(u"Copying extra file '%1' failed.").arg(copyFileStr);
        }
    }

    return newQmlObjectNode;
}

QmlVisualNode QmlVisualNode::createQml3DNode(AbstractView *view,
                                             const ItemLibraryEntry &itemLibraryEntry,
                                             qint32 sceneRootId, const QVector3D &position,
                                             bool createInTransaction)
{
    NodeAbstractProperty sceneNodeProperty = sceneRootId != -1 ? findSceneNodeProperty(view, sceneRootId)
                                                               : view->rootModelNode().defaultNodeAbstractProperty();

    QTC_ASSERT(sceneNodeProperty.isValid(), return {});

    return createQmlObjectNode(view, itemLibraryEntry, position, sceneNodeProperty, createInTransaction).modelNode();
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
    return node.isValid() && node.metaInfo().isFlowViewFlowTransition();
}

bool QmlVisualNode::isFlowDecision(const ModelNode &node)
{
    return node.isValid() && node.metaInfo().isFlowViewFlowDecision();
}

bool QmlVisualNode::isFlowWildcard(const ModelNode &node)
{
    return node.isValid() && node.metaInfo().isFlowViewFlowWildcard();
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
        return {};

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
        return {};

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
        return {};

    ModelNode newState = QmlModelState::createQmlState(
                modelNode().view(), {{PropertyName("name"), QVariant(name)}});
    modelNode().nodeListProperty("states").reparentHere(newState);

    return newState;
}

void QmlModelStateGroup::removeState(const QString &name)
{
    if (!modelNode().isValid())
        return;

    if (state(name).isValid())
        state(name).modelNode().destroy();
}

QmlModelState QmlModelStateGroup::state(const QString &name) const
{
    if (!modelNode().isValid())
        return {};

    if (modelNode().property("states").isNodeListProperty()) {
        for (const ModelNode &node : modelNode().nodeListProperty("states").toModelNodeList()) {
            if (QmlModelState(node).name() == name)
                return node;
        }
    }
    return QmlModelState();
}

bool QmlVisualNode::Position::is3D() const
{
    return m_is3D;
}

QList<QPair<PropertyName, QVariant> > QmlVisualNode::Position::propertyPairList() const
{
    QList<QPair<PropertyName, QVariant> > propertyPairList;

    if (m_is3D) {
        if (!qFuzzyIsNull(x()))
            propertyPairList.append({"x", QVariant{x()}});
        if (!qFuzzyIsNull(y()))
            propertyPairList.append({"y", QVariant{y()}});
        if (!qFuzzyIsNull(z()))
            propertyPairList.append({"z", QVariant{z()}});
    } else {
        if (const int intX = qRound(x()))
            propertyPairList.append({"x", QVariant(intX)});
        if (const int intY = qRound(y()))
            propertyPairList.append({"y", QVariant(intY)});
    }

    return propertyPairList;
}

} //QmlDesigner
