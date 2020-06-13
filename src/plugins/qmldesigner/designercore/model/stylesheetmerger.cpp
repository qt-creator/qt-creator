/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include "stylesheetmerger.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <invalididexception.h>
#include <invalidmodelnodeexception.h>
#include <invalidreparentingexception.h>
#include <modelmerger.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <variantproperty.h>

#include <QQueue>
#include <QRegularExpression>

namespace {

QPoint pointForModelNode(const QmlDesigner::ModelNode &node)
{
    int x = 0;
    if (node.hasVariantProperty("x"))
        x = node.variantProperty("x").value().toInt();

    int y = 0;
    if (node.hasVariantProperty("y"))
        y = node.variantProperty("y").value().toInt();

    return QPoint(x, y);
}

QPoint parentPosition(const QmlDesigner::ModelNode &node)
{
    QPoint p;

    QmlDesigner::ModelNode currentNode = node;
    while (currentNode.hasParentProperty()) {
        currentNode = currentNode.parentProperty().parentModelNode();
        p += pointForModelNode(currentNode);
    }

    return p;
}

bool isTextAlignmentProperty(const QmlDesigner::VariantProperty &property)
{
    return property.name() == "horizontalAlignment"
                || property.name() == "verticalAlignment"
                || property.name() == "elide";
}
} // namespace

namespace QmlDesigner {

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

void StylesheetMerger::syncNodeProperties(ModelNode &outputNode, const ModelNode &inputNode, bool skipDuplicates)
{
    for (const NodeProperty &nodeProperty : inputNode.nodeProperties()) {
        ModelNode oldNode = nodeProperty.modelNode();
        if (m_templateView->hasId(oldNode.id()) && skipDuplicates)
            continue;
        ModelNode newNode = createReplacementNode(oldNode, oldNode);
        // cache the property name as removing it will invalidate it
        PropertyName propertyName = nodeProperty.name();
        // remove property first to prevent invalid reparenting situation
        outputNode.removeProperty(propertyName);
        outputNode.nodeProperty(propertyName).reparentHere(newNode);
    }
}

void StylesheetMerger::syncNodeListProperties(ModelNode &outputNode, const ModelNode &inputNode, bool skipDuplicates)
{
    for (const NodeListProperty &nodeListProperty : inputNode.nodeListProperties()) {
        for (ModelNode node : nodeListProperty.toModelNodeList()) {
            if (m_templateView->hasId(node.id()) && skipDuplicates)
                continue;
            ModelNode newNode = createReplacementNode(node, node);
            outputNode.nodeListProperty(nodeListProperty.name()).reparentHere(newNode);
        }
    }
}

void StylesheetMerger::syncVariantProperties(ModelNode &outputNode, const ModelNode &inputNode)
{
    for (const VariantProperty &variantProperty : inputNode.variantProperties()) {
        outputNode.variantProperty(variantProperty.name()).setValue(variantProperty.value());
    }
}

void StylesheetMerger::syncAuxiliaryProperties(ModelNode &outputNode, const ModelNode &inputNode)
{
    auto tmp = inputNode.auxiliaryData();
    for (auto iter = tmp.begin(); iter != tmp.end(); ++iter)
        outputNode.setAuxiliaryData(iter.key(), iter.value());
}

void StylesheetMerger::syncBindingProperties(ModelNode &outputNode, const ModelNode &inputNode)
{
    for (const BindingProperty &bindingProperty : inputNode.bindingProperties()) {
        outputNode.bindingProperty(bindingProperty.name()).setExpression(bindingProperty.expression());
    }
}

void StylesheetMerger::syncId(ModelNode &outputNode, ModelNode &inputNode)
{
    if (!inputNode.id().isEmpty()) {
        QString id = inputNode.id();
        QString renamedId = m_idReplacementHash.value(inputNode.id());
        inputNode.setIdWithoutRefactoring(renamedId);
        outputNode.setIdWithoutRefactoring(id);
    }
}

void StylesheetMerger::setupIdRenamingHash()
{
    for (const ModelNode &node : m_templateView->rootModelNode().allSubModelNodesAndThisNode()) {
        if (!node.id().isEmpty()) {
            QString newId = node.id();
            QString baseId;
            int number = 1;
            splitIdInBaseNameAndNumber(newId, &baseId, &number);

            while (m_templateView->hasId(newId) || m_idReplacementHash.values().contains(newId)) {
                newId = "stylesheet_auto_merge_" + baseId + QString::number(number);
                number++;
            }

            m_idReplacementHash.insert(node.id(), newId);
        }
    }
}

ModelNode StylesheetMerger::createReplacementNode(const ModelNode& styleNode, ModelNode &modelNode)
{
    QList<QPair<PropertyName, QVariant> > propertyList;
    QList<QPair<PropertyName, QVariant> > auxPropertyList;
    NodeMetaInfo nodeMetaInfo = m_templateView->model()->metaInfo(styleNode.type());

    for (const VariantProperty &variantProperty : modelNode.variantProperties()) {
        if (!nodeMetaInfo.hasProperty(variantProperty.name()))
            continue;
        if (isTextAlignmentProperty(variantProperty) && !m_options.preserveTextAlignment && !styleNode.hasProperty(variantProperty.name()))
            continue;
        propertyList.append(QPair<PropertyName, QVariant>(variantProperty.name(), variantProperty.value()));
    }
    ModelNode newNode(m_templateView->createModelNode(styleNode.type(), nodeMetaInfo.majorVersion(), nodeMetaInfo.minorVersion(),
                                                      propertyList, auxPropertyList, styleNode.nodeSource(), styleNode.nodeSourceType()));

    syncAuxiliaryProperties(newNode, modelNode);
    syncBindingProperties(newNode, modelNode);
    syncId(newNode, modelNode);
    syncNodeProperties(newNode, modelNode);
    syncNodeListProperties(newNode, modelNode);

    return newNode;
}

StylesheetMerger::StylesheetMerger(AbstractView *templateView, AbstractView *styleView)
    : m_templateView(templateView)
    , m_styleView(styleView)
{
}

bool StylesheetMerger::idExistsInBothModels(const QString& id)
{
    return m_templateView->hasId(id) && m_styleView->hasId(id);
}

void StylesheetMerger::preprocessStyleSheet()
{
    try {
        RewriterTransaction transaction(m_styleView, "preprocess-stylesheet");
        for (const ModelNode &currentStyleNode : m_styleView->rootModelNode().directSubModelNodes()) {
            QString id = currentStyleNode.id();

            if (!idExistsInBothModels(id))
                continue;

            ModelNode templateNode = m_templateView->modelNodeForId(id);
            NodeAbstractProperty templateParentProperty = templateNode.parentProperty();
            if (!templateNode.hasParentProperty()
                    || templateParentProperty.parentModelNode().isRootNode())
                continue;

            ModelNode templateParentNode = templateParentProperty.parentModelNode();
            const QString parentId = templateParentNode.id();
            if (!idExistsInBothModels(parentId))
                continue;

            // Only get the position properties as the node should have a global
            // position in the style sheet.
            const QPoint oldGlobalPos = pointForModelNode(currentStyleNode);

            ModelNode newStyleParent = m_styleView->modelNodeForId(parentId);
            NodeListProperty newParentProperty = newStyleParent.defaultNodeListProperty();
            newParentProperty.reparentHere(currentStyleNode);

            // Get the parent position in global coordinates.
            QPoint parentGlobalPos = parentPosition(currentStyleNode);

            const QPoint newGlobalPos = oldGlobalPos - parentGlobalPos;

            currentStyleNode.variantProperty("x").setValue(newGlobalPos.x());
            currentStyleNode.variantProperty("y").setValue(newGlobalPos.y());

            int templateParentIndex = templateParentProperty.isNodeListProperty()
                    ? templateParentProperty.indexOf(templateNode) : -1;
            int styleParentIndex = newParentProperty.indexOf(currentStyleNode);
            if (templateParentIndex >= 0 && styleParentIndex != templateParentIndex)
                newParentProperty.slide(styleParentIndex, templateParentIndex);
        }
        transaction.commit();
    }catch (InvalidIdException &ide) {
        qDebug().noquote() << "Invalid id exception while preprocessing the style sheet.";
        ide.createWarning();
    } catch (InvalidReparentingException &rpe) {
        qDebug().noquote() << "Invalid reparenting exception while preprocessing the style sheet.";
        rpe.createWarning();
    } catch (InvalidModelNodeException &mne) {
        qDebug().noquote() << "Invalid model node exception while preprocessing the style sheet.";
        mne.createWarning();
    } catch (Exception &e) {
        qDebug().noquote() << "Exception while preprocessing the style sheet.";
        e.createWarning();
    }

}

void StylesheetMerger::replaceNode(ModelNode &replacedNode, ModelNode &newNode)
{
    NodeListProperty replacedNodeParent;
    ModelNode parentModelNode = replacedNode.parentProperty().parentModelNode();
    if (replacedNode.parentProperty().isNodeListProperty())
        replacedNodeParent = replacedNode.parentProperty().toNodeListProperty();
    bool isNodeProperty = false;

    PropertyName reparentName;
    for (const NodeProperty &prop : parentModelNode.nodeProperties()) {
        if (prop.modelNode().id() == replacedNode.id()) {
            isNodeProperty = true;
            reparentName = prop.name();
        }
    }
    ReparentInfo info;
    info.parentIndex = replacedNodeParent.isValid() ? replacedNodeParent.indexOf(replacedNode) : -1;
    info.templateId = replacedNode.id();
    info.templateParentId = parentModelNode.id();
    info.generatedId = newNode.id();

    if (!isNodeProperty) {
        replacedNodeParent.reparentHere(newNode);
        replacedNode.destroy();
        info.alreadyReparented = true;
    } else {
        parentModelNode.removeProperty(reparentName);
        parentModelNode.nodeProperty(reparentName).reparentHere(newNode);
    }
    m_reparentInfoHash.insert(newNode.id(), info);
}

void StylesheetMerger::replaceRootNode(ModelNode& templateRootNode)
{
    try {
        RewriterTransaction transaction(m_templateView, "replace-root-node");
        ModelMerger merger(m_templateView);
        QString rootId = templateRootNode.id();
        // If we shall replace the root node of the template with the style,
        // we first replace the whole model.
        ModelNode rootReplacer = m_styleView->modelNodeForId(rootId);
        merger.replaceModel(rootReplacer);

        // Then reset the id to the old root's one.
        ModelNode newRoot = m_templateView->rootModelNode();
        newRoot.setIdWithoutRefactoring(rootId);
        transaction.commit();
    }  catch (InvalidIdException &ide) {
        qDebug().noquote() << "Invalid id exception while replacing root node of template.";
        ide.createWarning();
    } catch (InvalidReparentingException &rpe) {
        qDebug().noquote() << "Invalid reparenting exception while replacing root node of template.";
        rpe.createWarning();
    } catch (InvalidModelNodeException &mne) {
        qDebug().noquote() << "Invalid model node exception while replacing root node of template.";
        mne.createWarning();
    } catch (Exception &e) {
        qDebug().noquote() << "Exception while replacing root node of template.";
        e.createWarning();
    }
}

// Move the newly created nodes to the correct position in the parent node
void StylesheetMerger::adjustNodeIndex(ModelNode &node)
{
    if (!m_reparentInfoHash.contains(node.id()))
        return;

    ReparentInfo info = m_reparentInfoHash.value(node.id());
    if (info.parentIndex < 0)
        return;

    if (!node.parentProperty().isNodeListProperty())
        return;

    NodeListProperty parentListProperty = node.parentProperty().toNodeListProperty();
    int currentIndex = parentListProperty.indexOf(node);
    if (currentIndex == info.parentIndex)
        return;

    parentListProperty.slide(currentIndex, info.parentIndex);
}

void StylesheetMerger::applyStyleProperties(ModelNode &templateNode, const ModelNode &styleNode)
{
    QRegExp regEx("[a-z]", Qt::CaseInsensitive);
    for (const VariantProperty &variantProperty : styleNode.variantProperties()) {
        if (templateNode.hasBindingProperty(variantProperty.name())) {
            // if the binding does not contain any alpha letters (i.e. binds to a term rather than a property,
            // replace it with the corresponding variant property.
            if (!templateNode.bindingProperty(variantProperty.name()).expression().contains(regEx)) {
                templateNode.removeProperty(variantProperty.name());
                templateNode.variantProperty(variantProperty.name()).setValue(variantProperty.value());
            }
        } else {
            if (variantProperty.holdsEnumeration()) {
                templateNode.variantProperty(variantProperty.name()).setEnumeration(variantProperty.enumeration().toEnumerationName());
            } else {
                templateNode.variantProperty(variantProperty.name()).setValue(variantProperty.value());
            }
        }
    }
    syncBindingProperties(templateNode, styleNode);
    syncNodeProperties(templateNode, styleNode, true);
    syncNodeListProperties(templateNode, styleNode, true);
}

static void removePropertyIfExists(ModelNode node, const PropertyName &propertyName)
{
    if (node.hasProperty(propertyName))
        node.removeProperty(propertyName);

}

void StylesheetMerger::parseTemplateOptions()
{
    if (!m_templateView->hasId(QStringLiteral("qds_stylesheet_merger_options")))
        return;

    ModelNode optionsNode = m_templateView->modelNodeForId(QStringLiteral("qds_stylesheet_merger_options"));
    if (optionsNode.hasVariantProperty("preserveTextAlignment")) {
        m_options.preserveTextAlignment = optionsNode.variantProperty("preserveTextAlignment").value().toBool();
    }
    if (optionsNode.hasVariantProperty("useStyleSheetPositions")) {
        m_options.useStyleSheetPositions = optionsNode.variantProperty("useStyleSheetPositions").value().toBool();
    }
    try {
        RewriterTransaction transaction(m_templateView, "remove-options-node");
        optionsNode.destroy();
        transaction.commit();
    } catch (InvalidIdException &ide) {
        qDebug().noquote() << "Invalid id exception while removing options from template.";
        ide.createWarning();
    } catch (InvalidReparentingException &rpe) {
        qDebug().noquote() << "Invalid reparenting exception while removing options from template.";
        rpe.createWarning();
    } catch (InvalidModelNodeException &mne) {
        qDebug().noquote() << "Invalid model node exception while removing options from template.";
        mne.createWarning();
    } catch (Exception &e) {
        qDebug().noquote() << "Exception while removing options from template.";
        e.createWarning();
    }
}

void StylesheetMerger::merge()
{
    ModelNode templateRootNode = m_templateView->rootModelNode();
    ModelNode styleRootNode = m_styleView->rootModelNode();

    // first, look if there are any options present in the template
    parseTemplateOptions();

    // second, build up the hierarchy in the style sheet as we have it in the template
    preprocessStyleSheet();

    // build a hash of generated replacement ids
    setupIdRenamingHash();

    //in case we are replacing the root node, just do that and exit
    if (m_styleView->hasId(templateRootNode.id())) {
        replaceRootNode(templateRootNode);
        return;
    }

    QQueue<ModelNode> replacementNodes;

    QList<ModelNode> directRootSubNodes = styleRootNode.directSubModelNodes();
    if (directRootSubNodes.length() == 0 && m_templateView->hasId(styleRootNode.id())) {
        // if the style sheet has only one node, just replace that one
        replacementNodes.enqueue(styleRootNode);
    }
    // otherwise, the nodes to replace are the direct sub nodes of the style sheet's root
    for (const ModelNode &subNode : styleRootNode.allSubModelNodes()) {
        if (m_templateView->hasId(subNode.id())) {
            replacementNodes.enqueue(subNode);
        }
    }

    for (const ModelNode &currentNode : replacementNodes) {

        bool hasPos = false;

        // create the replacement nodes for the styled nodes
        {
            try {
                RewriterTransaction transaction(m_templateView, "create-replacement-node");

                ModelNode replacedNode = m_templateView->modelNodeForId(currentNode.id());
                hasPos = replacedNode.hasProperty("x") || replacedNode.hasProperty("y");

                ModelNode replacementNode = createReplacementNode(currentNode, replacedNode);

                replaceNode(replacedNode, replacementNode);
                transaction.commit();
            } catch (InvalidIdException &ide) {
                qDebug().noquote() << "Invalid id exception while replacing template node";
                ide.createWarning();
                continue;
            } catch (InvalidReparentingException &rpe) {
                qDebug().noquote() << "Invalid reparenting exception while replacing template node";
                rpe.createWarning();
                continue;
            } catch (InvalidModelNodeException &mne) {
                qDebug().noquote() << "Invalid model node exception while replacing template node";
                mne.createWarning();
                continue;
            } catch (Exception &e) {
                qDebug().noquote() << "Exception while replacing template node.";
                e.createWarning();
                continue;
            }
        }
        // sync the properties from the stylesheet
        {
            try {
                RewriterTransaction transaction(m_templateView, "sync-style-node-properties");
                ModelNode templateNode = m_templateView->modelNodeForId(currentNode.id());
                applyStyleProperties(templateNode, currentNode);
                adjustNodeIndex(templateNode);

                /* This we want to do if the parent node in the style is not in the template */
                if (!currentNode.hasParentProperty() ||
                        !m_templateView->modelNodeForId(currentNode.parentProperty().parentModelNode().id()).isValid()) {

                    if (!hasPos && !m_options.useStyleSheetPositions) { //If template had postition retain it
                        removePropertyIfExists(templateNode, "x");
                        removePropertyIfExists(templateNode, "y");
                    }
                    if (templateNode.hasProperty("anchors.fill")) {
                        /* Unfortuntly there are cases were width and height have to be defined - see Button
                     * Most likely we need options for this */
                        //removePropertyIfExists(templateNode, "width");
                        //removePropertyIfExists(templateNode, "height");
                    }
                }
                transaction.commit();
            } catch (InvalidIdException &ide) {
                qDebug().noquote() << "Invalid id exception while syncing style properties to template";
                ide.createWarning();
                continue;
            } catch (InvalidReparentingException &rpe) {
                qDebug().noquote() << "Invalid reparenting exception while syncing style properties to template";
                rpe.createWarning();
                continue;
            } catch (InvalidModelNodeException &mne) {
                qDebug().noquote() << "Invalid model node exception while syncing style properties to template";
                mne.createWarning();
                continue;
            } catch (Exception &e) {
                qDebug().noquote() << "Exception while syncing style properties.";
                e.createWarning();
                continue;
            }
        }
    }
}
}
