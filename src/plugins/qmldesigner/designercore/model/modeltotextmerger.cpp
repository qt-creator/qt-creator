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

#include "modelnodepositionrecalculator.h"
#include "modeltotextmerger.h"
#include "qmltextgenerator.h"
#include "rewriteactioncompressor.h"
#include "rewriterview.h"

#include <qmljs/qmljsdocument.h>
#include <variantproperty.h>
#include <nodelistproperty.h>
#include <nodeproperty.h>
#include <textmodifier.h>

#include <QDebug>

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
        if (!isInHierarchy(property))
            continue;

        ModelNode containedModelNode;
        const int indentDepth = m_rewriterView->textModifier()->indentDepth();
        const QString propertyTextValue = QmlTextGenerator(getPropertyOrder(),
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
            Q_ASSERT(!"Unknown PropertyChangeFlags");
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
            if (!oldPropertyParent.isDefaultProperty() && oldPropertyParent.toNodeListProperty().toModelNodeList().size() == 0)
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
                                                  QmlTextGenerator(getPropertyOrder())(node),
                                                  propertyType(newPropertyParent),
                                                  node));
            break;

        case AbstractView::NoAdditionalChanges:
            schedule(new ChangePropertyRewriteAction(newPropertyParent,
                                                     QmlTextGenerator(getPropertyOrder())(node),
                                                     propertyType(newPropertyParent),
                                                     node));
            break;

        case AbstractView::EmptyPropertiesRemoved:
            break;

        default:
            Q_ASSERT(!"Unknown PropertyChange value");
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
        Q_ASSERT(!"Nodes do not belong to the same containing property");
}

RewriterView *ModelToTextMerger::view()
{
    return m_rewriterView;
}

void ModelToTextMerger::applyChanges()
{
    if (m_rewriteActions.isEmpty())
        return;

    dumpRewriteActions(QLatin1String("Before compression"));
    RewriteActionCompressor compress(getPropertyOrder());
    compress(m_rewriteActions);
    dumpRewriteActions(QLatin1String("After compression"));

    if (m_rewriteActions.isEmpty())
        return;

    Document::MutablePtr tmpDocument(Document::create(QLatin1String("<ModelToTextMerger>"), Document::QmlLanguage));
    tmpDocument->setSource(m_rewriterView->textModifier()->text());
    if (!tmpDocument->parseQml()) {
        qDebug() << "*** Possible problem: QML file wasn't parsed correctly.";
        qDebug() << "*** QML text:" << m_rewriterView->textModifier()->text();

        QString errorMessage = QLatin1String("Error while rewriting");
        if (!tmpDocument->diagnosticMessages().isEmpty())
            errorMessage = tmpDocument->diagnosticMessages().first().message;

        m_rewriterView->enterErrorState(errorMessage);
        return;
    }

    TextModifier *textModifier = m_rewriterView->textModifier();

    try {
        ModelNodePositionRecalculator positionRecalculator(m_rewriterView->positionStorage(), m_rewriterView->positionStorage()->modelNodes());
        positionRecalculator.connectTo(textModifier);

        QmlDesigner::QmlRefactoring refactoring(tmpDocument, *textModifier, getPropertyOrder());

        textModifier->deactivateChangeSignals();
        textModifier->startGroup();

        for (int i = 0; i < m_rewriteActions.size(); ++i) {
            RewriteAction* action = m_rewriteActions.at(i);
            if (DebugRewriteActions)
                qDebug() << "Next rewrite action:" << qPrintable(action->info());

            ModelNodePositionStorage *positionStore = m_rewriterView->positionStorage();
            bool success = action->execute(refactoring, *positionStore);

            if (success) {
                textModifier->flushGroup();
                success = refactoring.reparseDocument();
            }
            // don't merge these two if statements, because the previous then-part changes the value
            // of "success" !
            if (!success) {
                m_rewriterView->enterErrorState(QLatin1String("Error rewriting document"));

                if (true || DebugRewriteActions) {
                    qDebug() << "*** QML source code: ***";
                    qDebug() << qPrintable(textModifier->text());
                    qDebug() << "*** End of QML source code. ***";
                }

                break;
            }
        }

        qDeleteAll(m_rewriteActions);
        m_rewriteActions.clear();

        reindent(positionRecalculator.dirtyAreas());

        textModifier->commitGroup();

        textModifier->reactivateChangeSignals();
    } catch (Exception &e) {
        m_rewriterView->enterErrorState(e.description());

        qDeleteAll(m_rewriteActions);
        m_rewriteActions.clear();
        textModifier->commitGroup();
        textModifier->reactivateChangeSignals();
    }
}

void ModelToTextMerger::reindent(const QMap<int, int> &dirtyAreas) const
{
    QList<int> offsets = dirtyAreas.keys();
    qSort(offsets);
    TextModifier *textModifier = m_rewriterView->textModifier();

    foreach (const int offset, offsets) {
        const int length = dirtyAreas[offset];
        textModifier->indent(offset, length);
    }
}

void ModelToTextMerger::schedule(RewriteAction *action)
{
    Q_ASSERT(action);

    m_rewriteActions.append(action);
}

QmlDesigner::QmlRefactoring::PropertyType ModelToTextMerger::propertyType(const AbstractProperty &property, const QString &textValue)
{
    if (property.isBindingProperty()) {
        QString val = textValue.trimmed();
        if (val.isEmpty())
            return QmlDesigner::QmlRefactoring::ObjectBinding;
        const QChar lastChar = val.at(val.size() - 1);
        if (lastChar == '}' || lastChar == ';')
            return QmlDesigner::QmlRefactoring::ObjectBinding;
        else
            return QmlDesigner::QmlRefactoring::ScriptBinding;
    } else if (property.isNodeListProperty())
        return QmlDesigner::QmlRefactoring::ArrayBinding;
    else if (property.isNodeProperty())
        return QmlDesigner::QmlRefactoring::ObjectBinding;
    else if (property.isVariantProperty())
        return QmlDesigner::QmlRefactoring::ScriptBinding;

    Q_ASSERT(!"cannot convert property type");
    return (QmlDesigner::QmlRefactoring::PropertyType) -1;
}

QStringList ModelToTextMerger::m_propertyOrder;

QStringList ModelToTextMerger::getPropertyOrder()
{
    if (m_propertyOrder.isEmpty()) {
        m_propertyOrder
                << QLatin1String("id")
                << QLatin1String("name")
                << QLatin1String("target")
                << QLatin1String("property")
                << QLatin1String("x")
                << QLatin1String("y")
                << QLatin1String("width")
                << QLatin1String("height")
                << QLatin1String("position")
                << QLatin1String("color")
                << QLatin1String("radius")
                << QLatin1String("text")
                << QString::null
                << QLatin1String("states")
                << QLatin1String("transitions")
                ;
    }

    return m_propertyOrder;
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
