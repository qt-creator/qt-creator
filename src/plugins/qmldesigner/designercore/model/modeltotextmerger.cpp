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
    foreach (const AbstractProperty &property, propertyList) {
        if (isInHierarchy(property) && !property.isDefaultProperty())
            schedule(new RemovePropertyRewriteAction(property));
    }
}

void ModelToTextMerger::propertiesChanged(const QList<AbstractProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    foreach (const AbstractProperty &property, propertyList) {

        ModelNode containedModelNode;
        const int indentDepth = m_rewriterView->textModifier()->indentDepth();
        const QString propertyTextValue = QmlTextGenerator(propertyOrder(),
                                                           indentDepth)(property);

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

void ModelToTextMerger::addImport(const Import &import)
{
    if (!import.isEmpty())
        schedule(new AddImportRewriteAction(import));
}

void ModelToTextMerger::removeImport(const Import &import)
{
    if (!import.isEmpty())
        schedule(new RemoveImportRewriteAction(import));
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
                                                  QmlTextGenerator(propertyOrder())(node),
                                                  propertyType(newPropertyParent),
                                                  node));
            break;

        case AbstractView::NoAdditionalChanges:
            schedule(new ChangePropertyRewriteAction(newPropertyParent,
                                                     QmlTextGenerator(propertyOrder())(node),
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

    dumpRewriteActions(QStringLiteral("Before compression"));
    RewriteActionCompressor compress(propertyOrder());
    compress(m_rewriteActions);
    dumpRewriteActions(QStringLiteral("After compression"));

    if (m_rewriteActions.isEmpty())
        return;

    m_rewriterView->emitCustomNotification(StartRewriterApply);

    Document::MutablePtr tmpDocument(Document::create(QStringLiteral("<ModelToTextMerger>"), Dialect::Qml));
    tmpDocument->setSource(m_rewriterView->textModifier()->text());
    if (!tmpDocument->parseQml()) {
        qDebug() << "*** Possible problem: QML file wasn't parsed correctly.";
        qDebug() << "*** QML text:" << m_rewriterView->textModifier()->text();

        QString errorMessage = QStringLiteral("Error while rewriting");
        if (!tmpDocument->diagnosticMessages().isEmpty())
            errorMessage = tmpDocument->diagnosticMessages().first().message;

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

        for (int i = 0; i < m_rewriteActions.size(); ++i) {
            RewriteAction* action = m_rewriteActions.at(i);
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

    foreach (const int offset, offsets) {
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
        PropertyName("position"),
        PropertyName("color"),
        PropertyName("radius"),
        PropertyName("text"),
        PropertyName(),
        PropertyName("states"),
        PropertyName("transitions")
    };

    return properties;
}

bool ModelToTextMerger::isInHierarchy(const AbstractProperty &property) {
    return property.isValid() && property.parentModelNode().isInHierarchy();
}

void ModelToTextMerger::dumpRewriteActions(const QString &msg)
{
    if (DebugRewriteActions) {
        qDebug() << "---->" << qPrintable(msg);

        foreach (RewriteAction *action, m_rewriteActions) {
            qDebug() << "-----" << qPrintable(action->info());
        }

        qDebug() << "<----" << qPrintable(msg);
    }
}
