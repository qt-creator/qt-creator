// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modeltotextmerger.h"
#include "modelnodepositionrecalculator.h"
#include "qmltextgenerator.h"
#include "rewriteactioncompressor.h"

#include <customnotifications.h>

#include <rewriterview.h>
#include <abstractproperty.h>
#include <nodeproperty.h>
#include <nodeabstractproperty.h>

#include <qmljs/parser/qmljsengine_p.h>
#include <utils/algorithm.h>

namespace {
    enum {
        DebugRewriteActions = 0
    };
}

using namespace QmlJS;
using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace Qt::StringLiterals;

ModelToTextMerger::ModelToTextMerger(RewriterView *reWriterView):
        m_rewriterView(reWriterView)
{
}

void ModelToTextMerger::nodeCreated(const ModelNode &/*createdNode*/)
{
    //the rewriter ignores model nodes outside of the hierachy
}

void ModelToTextMerger::nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange)
{
    if (!isInHierarchy(parentProperty))
        return;

    if (parentProperty.isDefaultProperty())
        schedule(new RemoveNodeRewriteAction(removedNode));
    else if (AbstractView::EmptyPropertiesRemoved == propertyChange)
        schedule(new RemovePropertyRewriteAction(parentProperty));
    else if (parentProperty.isNodeListProperty())
        schedule(new RemoveNodeRewriteAction(removedNode));
}

void ModelToTextMerger::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        // Default property that has actual binding/value should be removed
        if (isInHierarchy(property) && (!property.isDefaultProperty()
                                        || property.isBindingProperty()
                                        || property.isVariantProperty()
                                        || property.isNodeProperty())) {
            schedule(new RemovePropertyRewriteAction(property));
        }
    }
}

void ModelToTextMerger::propertiesChanged(const QList<AbstractProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    const TextEditor::TabSettings tabSettings = m_rewriterView->textModifier()->tabSettings();
    for (const AbstractProperty &property : propertyList) {

        ModelNode containedModelNode;
        const QString propertyTextValue = QmlTextGenerator(propertyOrder(),
                                                           tabSettings,
                                                           tabSettings.m_indentSize)(property);

        switch (propertyChange) {
        case AbstractView::PropertiesAdded:
            if (property.isNodeProperty())
                containedModelNode = property.toNodeProperty().modelNode();

            schedule(new AddPropertyRewriteAction(property,
                                                  propertyTextValue,
                                                  propertyType(property, propertyTextValue),
                                                  containedModelNode));
            break;

        case AbstractView::NoAdditionalChanges:
            if (property.isNodeProperty())
                containedModelNode = property.toNodeProperty().modelNode();

            schedule(new ChangePropertyRewriteAction(property,
                                                     propertyTextValue,
                                                     propertyType(property, propertyTextValue),
                                                     containedModelNode));
            break;

        case AbstractView::EmptyPropertiesRemoved:
            break;

        default:
            Q_ASSERT(false); //Unknown PropertyChange flag
        }
    }
}

void ModelToTextMerger::nodeTypeChanged(const ModelNode &node,const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    if (!node.isInHierarchy())
        return;

    // TODO: handle the majorVersion and the minorVersion

    schedule(new ChangeTypeRewriteAction(node));
}

void ModelToTextMerger::addImports(const Imports &imports)
{
    for (const Import &import : imports) {
        if (!import.isEmpty())
            schedule(new AddImportRewriteAction(import));
    }
}

void ModelToTextMerger::removeImports(const Imports &imports)
{
    for (const Import &import : imports) {
        if (!import.isEmpty())
            schedule(new RemoveImportRewriteAction(import));
    }
}

void ModelToTextMerger::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    if (isInHierarchy(oldPropertyParent) && isInHierarchy(newPropertyParent)) { // the node is moved
        schedule(new ReparentNodeRewriteAction(node,
                                               oldPropertyParent,
                                               newPropertyParent,
                                               propertyType(newPropertyParent)));
    } else if (isInHierarchy(oldPropertyParent) && !isInHierarchy(newPropertyParent)) { // the node is removed from hierarchy
        if (oldPropertyParent.isNodeProperty()) {
            // ignore, the subsequent remove property will take care of all
        } else if (oldPropertyParent.isNodeListProperty()) {
            if (!oldPropertyParent.isDefaultProperty() && oldPropertyParent.count() == 0)
                schedule(new RemovePropertyRewriteAction(oldPropertyParent));
            else
                schedule(new RemoveNodeRewriteAction(node));
        } else {
            schedule(new RemoveNodeRewriteAction(node));
        }
    } else if (!isInHierarchy(oldPropertyParent) && isInHierarchy(newPropertyParent)) { // the node is inserted into to hierarchy
        switch (propertyChange) {
        case AbstractView::PropertiesAdded:
            schedule(new AddPropertyRewriteAction(newPropertyParent,
                                                  QmlTextGenerator(propertyOrder(),
                                                                   m_rewriterView->textModifier()
                                                                       ->tabSettings())(node),
                                                  propertyType(newPropertyParent),
                                                  node));
            break;

        case AbstractView::NoAdditionalChanges:
            schedule(new ChangePropertyRewriteAction(newPropertyParent,
                                                     QmlTextGenerator(propertyOrder(),
                                                                      m_rewriterView->textModifier()
                                                                          ->tabSettings())(node),
                                                     propertyType(newPropertyParent),
                                                     node));
            break;

        case AbstractView::EmptyPropertiesRemoved:
            break;

        default:
            Q_ASSERT(false); //Unknown PropertyChange value
        }
    } else {
        // old is outside of hierarchy, new is outside of hierarchy, so who cares?
    }
}

void ModelToTextMerger::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    if (!node.isInHierarchy())
        return;

    schedule(new ChangeIdRewriteAction(node, oldId, newId));
}

void ModelToTextMerger::nodeSlidAround(const ModelNode &movingNode, const ModelNode &inFrontOfNode)
{
    if (!inFrontOfNode.isValid() || movingNode.parentProperty() == inFrontOfNode.parentProperty())
        schedule(new MoveNodeRewriteAction(movingNode, inFrontOfNode));
    else
        Q_ASSERT(false); //Nodes do not belong to the same containing property
}

RewriterView *ModelToTextMerger::view()
{
    return m_rewriterView;
}

void ModelToTextMerger::applyChanges()
{
    if (m_rewriteActions.isEmpty())
        return;

    dumpRewriteActions(u"Before compression");
    RewriteActionCompressor compress(propertyOrder(), m_rewriterView->positionStorage());
    compress(m_rewriteActions, m_rewriterView->textModifier()->tabSettings());
    dumpRewriteActions(u"After compression");

    if (m_rewriteActions.isEmpty())
        return;

    m_rewriterView->emitCustomNotification(StartRewriterApply);

    Document::MutablePtr tmpDocument(
        Document::create(Utils::FilePath::fromString("<ModelToTextMerger>"), Dialect::Qml));
    tmpDocument->setSource(m_rewriterView->textModifier()->text());
    if (!tmpDocument->parseQml()) {
        qDebug() << "*** Possible problem: QML file wasn't parsed correctly.";
        qDebug() << "*** QML text:" << m_rewriterView->textModifier()->text();

        QString errorMessage = "Error while rewriting"_L1;
        if (!tmpDocument->diagnosticMessages().isEmpty())
            errorMessage = tmpDocument->diagnosticMessages().constFirst().message;

        m_rewriterView->enterErrorState(errorMessage);
        return;
    }

    TextModifier *textModifier = m_rewriterView->textModifier();

    try {
        bool reindentAllFlag = false;

        ModelNodePositionRecalculator positionRecalculator(m_rewriterView->positionStorage(), m_rewriterView->positionStorage()->modelNodes());
        positionRecalculator.connectTo(textModifier);

        QmlRefactoring refactoring(tmpDocument, *textModifier, propertyOrder());

        textModifier->deactivateChangeSignals();
        textModifier->startGroup();

        for (auto action : std::as_const(m_rewriteActions)) {
            if (DebugRewriteActions)
                qDebug() << "Next rewrite action:" << qPrintable(action->info());

            if (action->asReparentNodeRewriteAction())
                reindentAllFlag = true; /*If a node is reparented we indent all,
                                          because reparenting can have side effects
                                          regarding indentation
                                          to otherwise untouched nodes.
                                          */

            ModelNodePositionStorage *positionStore = m_rewriterView->positionStorage();
            bool success = action->execute(refactoring, *positionStore);

            if (success) {
                textModifier->flushGroup();
                success = refactoring.reparseDocument();
            }
            // don't merge these two if statements, because the previous then-part changes the value
            // of "success" !
            if (!success) {
                m_rewriterView->enterErrorState(QStringLiteral("Error rewriting document"));

                if (DebugRewriteActions) {
                    qDebug() << "*** QML source code: ***";
                    qDebug() << qPrintable(textModifier->text());
                    qDebug() << "*** End of QML source code. ***";
                }

                break;
            }
        }

        qDeleteAll(m_rewriteActions);
        m_rewriteActions.clear();

        if (reindentAllFlag)
            reindentAll();
        else
            reindent(positionRecalculator.dirtyAreas());

        textModifier->commitGroup();

        textModifier->reactivateChangeSignals();
    } catch (const Exception &e) {
        m_rewriterView->enterErrorState(e.description());

        qDeleteAll(m_rewriteActions);
        m_rewriteActions.clear();
        textModifier->commitGroup();
        textModifier->reactivateChangeSignals();
    }

    m_rewriterView->emitCustomNotification(EndRewriterApply);
}

void ModelToTextMerger::reindent(const QMap<int, int> &dirtyAreas) const
{
    QList<int> offsets = dirtyAreas.keys();
    Utils::sort(offsets);
    TextModifier *textModifier = m_rewriterView->textModifier();

    for (const int offset : std::as_const(offsets)) {
        const int length = dirtyAreas[offset];
        textModifier->indent(offset, length);
    }
}

void ModelToTextMerger::reindentAll() const
{
    TextModifier *textModifier = m_rewriterView->textModifier();
    textModifier->indent(0, textModifier->text().length() - 1);
}

void ModelToTextMerger::schedule(RewriteAction *action)
{
    Q_ASSERT(action);

    m_rewriteActions.append(action);
}

QmlRefactoring::PropertyType ModelToTextMerger::propertyType(const AbstractProperty &property, const QString &textValue)
{
    if (property.isBindingProperty() || property.isSignalHandlerProperty()) {
        QString val = textValue.trimmed();
        if (val.isEmpty())
            return QmlRefactoring::ObjectBinding;
        const QChar lastChar = val.at(val.size() - 1);
        if (lastChar == '}' || lastChar == ';')
            return QmlRefactoring::ObjectBinding;
        else
            return QmlRefactoring::ScriptBinding;
    } else if (property.isNodeListProperty())
        return QmlRefactoring::ArrayBinding;
    else if (property.isNodeProperty())
        return QmlRefactoring::ObjectBinding;
    else if (property.isVariantProperty())
        return QmlRefactoring::ScriptBinding;
    else if (property.isSignalDeclarationProperty())
        return QmlRefactoring::ScriptBinding;

    Q_ASSERT(false); //Cannot convert property type
    return QmlRefactoring::Invalid;
}

PropertyNameList ModelToTextMerger::propertyOrder()
{
    static const PropertyNameList properties = {
        PropertyName("id"),
        PropertyName("name"),
        PropertyName("target"),
        PropertyName("property"),
        PropertyName("x"),
        PropertyName("y"),
        PropertyName("width"),
        PropertyName("height"),
        PropertyName("opacity"),
        PropertyName("visible"),
        PropertyName("position"),
        PropertyName("color"),
        PropertyName("radius"),
        PropertyName("text"),
        PropertyName("elide"),
        PropertyName("border.color"),
        PropertyName("border.width"),
        PropertyName("anchors.verticalCenter"),
        PropertyName("anchors.left"),
        PropertyName("anchors.right"),
        PropertyName("anchors.top"),
        PropertyName("anchors.bottom"),
        PropertyName("anchors.fill"),
        PropertyName("anchors.margins"),
        PropertyName("font.letterSpacing"),
        PropertyName("font.pixelSize"),
        PropertyName("horizontalAlignment"),
        PropertyName("verticalAlignment"),
        PropertyName("source"),
        PropertyName("lineHeight"),
        PropertyName("lineHeightMode"),
        PropertyName("wrapMode"),
        PropertyName(),
        PropertyName("states"),
        PropertyName("to"),
        PropertyName("from"),
        PropertyName("transitions")
    };

    return properties;
}

bool ModelToTextMerger::isInHierarchy(const AbstractProperty &property) {
    return property.isValid() && property.parentModelNode().isInHierarchy();
}

void ModelToTextMerger::dumpRewriteActions(QStringView msg)
{
    if (DebugRewriteActions) {
        qDebug() << "---->" << msg;

        for (RewriteAction *action : std::as_const(m_rewriteActions)) {
            qDebug() << "-----" << qPrintable(action->info());
        }

        qDebug() << "<----" << msg;
    }
}
