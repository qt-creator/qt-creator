// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "stylesheetmerger.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <modelmerger.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <plaintexteditmodifier.h>
#include <rewriterview.h>
#include <variantproperty.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QPlainTextEdit>
#include <QQueue>
#include <QRegularExpression>

#include <memory>

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

void StylesheetMerger::syncNodeProperties(ModelNode &outputNode, const ModelNode &inputNode, bool skipDuplicates)
{
    for (const NodeProperty &nodeProperty : inputNode.nodeProperties()) {
        ModelNode oldNode = nodeProperty.modelNode();
        if (m_templateView->hasId(oldNode.id()) && skipDuplicates)
            continue;
        ModelNode newNode = createReplacementNode(oldNode, oldNode);
        // cache the property name as removing it will invalidate it
        PropertyNameView propertyName = nodeProperty.name();
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
    for (const auto &[key, value] : inputNode.auxiliaryData())
        outputNode.setAuxiliaryData(AuxiliaryDataKeyView{key}, value);
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
    const QList<ModelNode> &nodes = m_templateView->rootModelNode().allSubModelNodesAndThisNode();
    for (const ModelNode &node : nodes) {
        if (!node.id().isEmpty()) {
            QString newId = node.id();
            QString baseId;
            int number = 1;
            splitIdInBaseNameAndNumber(newId, &baseId, &number);

            while (m_templateView->hasId(newId) || std::find(m_idReplacementHash.cbegin(),
                        m_idReplacementHash.cend(), newId) != m_idReplacementHash.cend()) {
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
    NodeMetaInfo nodeMetaInfo = m_templateView->model()->metaInfo(styleNode.documentTypeRepresentation());

    for (const VariantProperty &variantProperty : modelNode.variantProperties()) {
        if (!nodeMetaInfo.hasProperty(variantProperty.name()))
            continue;
        if (isTextAlignmentProperty(variantProperty) && !m_options.preserveTextAlignment && !styleNode.hasProperty(variantProperty.name()))
            continue;
        propertyList.emplace_back(variantProperty.name().toByteArray(), variantProperty.value());
    }

    ModelNode newNode(m_templateView->createModelNode(
        styleNode.documentTypeRepresentation(), propertyList, {}, styleNode.nodeSource(), styleNode.nodeSourceType()));

    syncAuxiliaryProperties(newNode, modelNode);
    syncBindingProperties(newNode, modelNode);
    syncId(newNode, modelNode);
    syncNodeProperties(newNode, modelNode);
    syncNodeListProperties(newNode, modelNode);
    mergeStates(newNode, modelNode);

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
    } catch (Exception &exception) {
        qDebug().noquote() << "Exception while preprocessing the style sheet.";
        qDebug() << exception;
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
            reparentName = prop.name().toByteArray();
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
    } catch (Exception &exception) {
        qDebug().noquote() << "Exception while replacing root node of template.";
        qDebug() << exception;
    }
}

// Move the newly created nodes to the correct position in the parent node
void StylesheetMerger::adjustNodeIndex(ModelNode &node)
{
    auto found = m_reparentInfoHash.find(node.id());
    if (found == m_reparentInfoHash.end())
        return;

    ReparentInfo info = *found;
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

void StylesheetMerger::applyStyleProperties(ModelNode &templateNode,
                                            const ModelNode &styleNode,
                                            bool isRootNode)
{
    // using isRootNode allows transferring custom properties that may have been added in Qt Bridge
    auto addProperty = [&templateNode, isRootNode](const VariantProperty &variantProperty) {
        if (isRootNode)
            templateNode.variantProperty(variantProperty.name())
                .setDynamicTypeNameAndValue(variantProperty.dynamicTypeName(),
                                            variantProperty.value());
        else
            templateNode.variantProperty(variantProperty.name()).setValue(variantProperty.value());
    };

    const QRegularExpression regEx("[a-z]", QRegularExpression::CaseInsensitiveOption);
    for (const VariantProperty &variantProperty : styleNode.variantProperties()) {
        if (templateNode.hasBindingProperty(variantProperty.name())) {
            // if the binding does not contain any alpha letters (i.e. binds to a term rather than a property,
            // replace it with the corresponding variant property.
            if (!templateNode.bindingProperty(variantProperty.name()).expression().contains(regEx)) {
                templateNode.removeProperty(variantProperty.name());
                addProperty(variantProperty);
            }
        } else {
            if (variantProperty.holdsEnumeration())
                templateNode.variantProperty(variantProperty.name())
                    .setEnumeration(variantProperty.enumeration().toEnumerationName());
            else
                addProperty(variantProperty);
        }
    }

    if (isRootNode)
        return;

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
    } catch (Exception &exception) {
        qDebug().noquote() << "Exception while removing options from template.";
        qDebug() << exception;
    }
}

void StylesheetMerger::syncStateNode(ModelNode &outputState, const ModelNode &inputState) const
{
    auto addProperty = [](ModelNode &n, const AbstractProperty &p) {
        if (n.hasProperty(p.name()))
            return; // Do not ovewrite. Only merge when property not defined in template.
        if (p.isBindingProperty())
            n.bindingProperty(p.name()).setExpression(p.toBindingProperty().expression());
        else
            n.variantProperty(p.name()).setValue(p.toVariantProperty().value());
    };

    addProperty(outputState, inputState.property("when"));
    addProperty(outputState, inputState.property("extend"));

    auto changeSetKey = [](const ModelNode &n) {
        return QString("%1::%2").arg(QString::fromUtf8(n.documentTypeRepresentation()),
                                     n.bindingProperty("target").expression());
    };

    // Collect change sets already defined in the output state.
    std::map<QString, ModelNode> outputChangeSets;
    for (ModelNode propChange : outputState.directSubModelNodes())
        outputChangeSets.insert({changeSetKey(propChange), propChange});

    // Merge the child nodes of the states i.e. AnchorChanges, PropertyChanges, etc.
    for (ModelNode inputChangeset : inputState.directSubModelNodes()) {
        const QString key = changeSetKey(inputChangeset);
        const auto itr = outputChangeSets.find(key);
        ModelNode changeSet;
        if (itr != outputChangeSets.end()) {
            changeSet = itr->second;
        } else {
            const QByteArray typeName = inputChangeset.documentTypeRepresentation();

            changeSet = m_templateView->createModelNode(typeName);

            outputState.nodeListProperty("changes").reparentHere(changeSet);
            outputChangeSets.insert({key, changeSet});
        }

        for (const auto &p : inputChangeset.properties())
            addProperty(changeSet, p);
    }
}

void StylesheetMerger::mergeStates(ModelNode &outputNode, const ModelNode &inputNode) const
{
    QMap<QString, ModelNode> outputStates;
    for (auto stateNode : outputNode.nodeListProperty("states").toModelNodeList()) {
        const QString name = stateNode.variantProperty("name").value().toString();
        if (name.isEmpty())
            continue;
        outputStates[name] = stateNode;
    }

    for (auto inputStateNode : inputNode.nodeListProperty("states").toModelNodeList()) {
        const QString name = inputStateNode.variantProperty("name").value().toString();
        try {
            if (outputStates.contains(name)) {
                syncStateNode(outputStates[name], inputStateNode);
                continue;
            }
            ModelMerger merger(m_templateView);
            ModelNode stateClone = merger.insertModel(inputStateNode);
            if (stateClone.isValid())
                outputNode.nodeListProperty("states").reparentHere(stateClone);
        } catch (Exception &exception) {
            qDebug().noquote() << "Exception while merging states.";
            qDebug() << exception;
        }
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

    // transfer custom root properties
    applyStyleProperties(templateRootNode, styleRootNode, true);

    //in case we are replacing the root node, just do that and exit
    if (m_styleView->hasId(templateRootNode.id())) {
        replaceRootNode(templateRootNode);
        return;
    }

    mergeStates(templateRootNode, styleRootNode);

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
            } catch (Exception &exception) {
                qDebug().noquote() << "Exception while replacing template node.";
                qDebug() << exception;
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
            } catch (Exception &exception) {
                qDebug().noquote() << "Exception while syncing style properties.";
                qDebug() << exception;
                continue;
            }
        }
    }
}

void StylesheetMerger::styleMerge(const Utils::FilePath &templateFile,
                                  Model *model,
                                  ModulesStorage &modulesStorage,
                                  ExternalDependenciesInterface &externalDependencies)
{
    const Utils::Result<QByteArray> res = templateFile.fileContents();

    QTC_ASSERT(res, return);
    const QString qmlTemplateString = QString::fromUtf8(*res);
    StylesheetMerger::styleMerge(qmlTemplateString, model, modulesStorage, externalDependencies);
}

void StylesheetMerger::styleMerge(const QString &qmlTemplateString,
                                  Model *model,
                                  ModulesStorage &modulesStorage,
                                  ExternalDependenciesInterface &externalDependencies)
{
    Model *parentModel = model;

    QTC_ASSERT(parentModel, return );

    auto templateModel = model->createModel({"Item"});

    Q_ASSERT(templateModel.get());

    templateModel->setFileUrl(parentModel->fileUrl());

    QPlainTextEdit textEditTemplate;
    QString imports;

    for (const Import &import : parentModel->imports()) {
        imports += QStringLiteral("import ") + import.toString(true) + QLatin1Char(';')
                   + QLatin1Char('\n');
    }

    textEditTemplate.setPlainText(imports + qmlTemplateString);
    NotIndentingTextEditModifier textModifierTemplate(textEditTemplate.document());

    std::unique_ptr<RewriterView> templateRewriterView = std::make_unique<RewriterView>(
        externalDependencies, modulesStorage, RewriterView::Amend);
    templateRewriterView->setTextModifier(&textModifierTemplate);
    templateModel->attachView(templateRewriterView.get());
    templateRewriterView->setCheckSemanticErrors(false);
    templateRewriterView->setPossibleImportsEnabled(false);

    ModelNode templateRootNode = templateRewriterView->rootModelNode();
    QTC_ASSERT(templateRootNode.isValid(), return );

    auto styleModel = model->createModel({"Item"});

    styleModel->setFileUrl(parentModel->fileUrl());

    QPlainTextEdit textEditStyle;
    RewriterView *parentRewriterView = parentModel->rewriterView();
    QTC_ASSERT(parentRewriterView, return );
    textEditStyle.setPlainText(parentRewriterView->textModifierContent());
    NotIndentingTextEditModifier textModifierStyle(textEditStyle.document());

    std::unique_ptr<RewriterView> styleRewriterView = std::make_unique<RewriterView>(
        externalDependencies, modulesStorage, RewriterView::Amend);
    styleRewriterView->setTextModifier(&textModifierStyle);
    styleModel->attachView(styleRewriterView.get());

    StylesheetMerger merger(templateRewriterView.get(), styleRewriterView.get());

    try {
        merger.merge();
    } catch (Exception &e) {
        e.showException();
    }

    try {
        parentRewriterView->textModifier()->textDocument()->setPlainText(
            templateRewriterView->textModifierContent());
    } catch (Exception &e) {
        e.showException();
    }
}
} // namespace QmlDesigner
