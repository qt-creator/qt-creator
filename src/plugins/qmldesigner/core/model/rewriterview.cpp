/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <filemanager/astobjecttextextractor.h>
#include <filemanager/objectlengthcalculator.h>
#include <filemanager/firstdefinitionfinder.h>
#include <customnotifications.h>

#include "rewriterview.h"
#include "textmodifier.h"
#include "texttomodelmerger.h"
#include "modelnodepositionstorage.h"
#include "modeltotextmerger.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "invalidmodelnodeexception.h"

using namespace QmlDesigner::Internal;

namespace QmlDesigner {

RewriterView::Error::Error():
        m_type(NoError),
        m_line(-1),
        m_column(-1)
{
}

RewriterView::Error::Error(const Exception &exception):
        m_type(InternalError),
        m_line(exception.line()),
        m_column(-1),
        m_description(exception.description()),
        m_url(exception.file())
{
}

RewriterView::Error::Error(const QmlError &qmlError):
        m_type(ParseError),
        m_line(qmlError.line()),
        m_column(qmlError.column()),
        m_description(qmlError.description()),
        m_url(qmlError.url())
{
}

QString RewriterView::Error::toString() const
{
    QString str;

    if (url().isValid())
        str = url().toString() + QLatin1Char(':');
    if (line() != -1)
        str += QString::number(line()) + QLatin1Char(':');
    if(column() != -1)
        str += QString::number(column()) + QLatin1Char(':');

    str += description();

    return str;
}

RewriterView::RewriterView(DifferenceHandling differenceHandling, QObject *parent):
        AbstractView(parent),
        m_differenceHandling(differenceHandling),
        m_modificationGroupActive(false),
        m_positionStorage(new ModelNodePositionStorage),
        m_modelToTextMerger(new Internal::ModelToTextMerger(this)),
        m_textToModelMerger(new Internal::TextToModelMerger(this)),
        m_textModifier(0),
        transactionLevel(0)
{
}

RewriterView::~RewriterView()
{
    delete m_positionStorage;
}

Internal::ModelToTextMerger *RewriterView::modelToTextMerger() const
{
    return m_modelToTextMerger.data();
}

Internal::TextToModelMerger *RewriterView::textToModelMerger() const
{
    return m_textToModelMerger.data();
}

void RewriterView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    ModelAmender differenceHandler(m_textToModelMerger.data());
    m_textToModelMerger->load(m_textModifier->text().toUtf8(), differenceHandler);
}

void RewriterView::modelAboutToBeDetached(Model * /*model*/)
{
    m_positionStorage->clear();
}

void RewriterView::nodeCreated(const ModelNode &createdNode)
{
    if (createdNode.type() == QLatin1String("Qt/Component"))
        setupComponent(createdNode);
    Q_ASSERT(textModifier());
    m_positionStorage->setNodeOffset(createdNode, ModelNodePositionStorage::INVALID_LOCATION);
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeCreated(createdNode);

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
}

void RewriterView::nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeRemoved(removedNode, parentProperty, propertyChange);

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::propertiesAdded(const ModelNode &/*node*/, const QList<AbstractProperty>& /*propertyList*/)
{
    Q_ASSERT(0);
}

void RewriterView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;


    foreach (const AbstractProperty &property, propertyList) {
        if (property.isDefaultProperty() && property.isNodeListProperty()) {
            m_removeDefaultPropertyTransaction = beginRewriterTransaction();

            foreach (const ModelNode &node, property.toNodeListProperty().toModelNodeList()) {
                modelToTextMerger()->nodeRemoved(node, property.toNodeAbstractProperty(), AbstractView::NoAdditionalChanges);
            }
        }
    }
}

void RewriterView::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->propertiesRemoved(propertyList);

    if (m_removeDefaultPropertyTransaction.isValid())
        m_removeDefaultPropertyTransaction.commit();

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    QList<AbstractProperty> usefulPropertyList;
    foreach (const VariantProperty &property, propertyList)
        usefulPropertyList.append(property);

    modelToTextMerger()->propertiesChanged(usefulPropertyList, propertyChange);

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    QList<AbstractProperty> usefulPropertyList;
    foreach (const BindingProperty &property, propertyList)
        usefulPropertyList.append(property);

    modelToTextMerger()->propertiesChanged(usefulPropertyList, propertyChange);

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    if (node.type() == QLatin1String("Qt/Component"))
        setupComponent(node);

    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeReparented(node, newPropertyParent, oldPropertyParent, propertyChange);

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/)
{
}

void RewriterView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeIdChanged(node, newId, oldId);

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::nodeSlidedToIndex(const NodeListProperty &listProperty, int newIndex, int /*oldIndex*/)
{ // FIXME: "slided" ain't no English, probably "slid" or "sliding" is meant...
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    const QList<ModelNode> nodes = listProperty.toModelNodeList();
    const ModelNode movingNode = nodes.at(newIndex);
    ModelNode trailingNode;
    if (newIndex + 1 < nodes.size())
        trailingNode = nodes.at(newIndex + 1);
    modelToTextMerger()->nodeSlidAround(movingNode, trailingNode);

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::nodeTypeChanged(const ModelNode &node,const QString &type, int majorVersion, int minorVersion)
{
     Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeTypeChanged(node, type, majorVersion, minorVersion);

    if (!isModificationGroupActive())
        modelToTextMerger()->applyChanges();
}

void RewriterView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> & /* nodeList */, const QList<QVariant> & /*data */)
{
    if (identifier == ("__start rewriter transaction__")) {
        transactionLevel++;
        setModificationGroupActive(true);
    }
    else if (identifier == ("__end rewriter transaction__")) {
        transactionLevel--;
        Q_ASSERT(transactionLevel >= 0);

    }
    if (transactionLevel == 0)
    {
        setModificationGroupActive(false);
        applyModificationGroupChanges();
    }
}

void RewriterView::selectedNodesChanged(const QList<ModelNode> & /* selectedNodeList, */, const QList<ModelNode> & /*lastSelectedNodeList */)
{
}

bool RewriterView::isModificationGroupActive() const
{
    return m_modificationGroupActive;
}

void RewriterView::setModificationGroupActive(bool active)
{
    m_modificationGroupActive = active;
}

TextModifier *RewriterView::textModifier() const
{
    return m_textModifier;
}

void RewriterView::setTextModifier(TextModifier *textModifier)
{
    if (m_textModifier)
        disconnect(m_textModifier, SIGNAL(textChanged()), this, SLOT(qmlTextChanged()));

    m_textModifier = textModifier;

    if (m_textModifier)
        connect(m_textModifier, SIGNAL(textChanged()), this, SLOT(qmlTextChanged()));
}

void RewriterView::applyModificationGroupChanges()
{
    Q_ASSERT(transactionLevel == 0);
    modelToTextMerger()->applyChanges();
}

void RewriterView::setupComponent(const ModelNode &node)
{
    Q_ASSERT(node.type() == QLatin1String("Qt/Component"));

    QString componentText = extractText(QList<ModelNode>() << node).value(node);

    if (componentText.isEmpty())
        return;

    QString result = "";
    if (componentText.contains("Component")) { //explicit component
        FirstDefinitionFinder firstDefinitionFinder(componentText);
        int offset = firstDefinitionFinder(0);
        ObjectLengthCalculator objectLengthCalculator(componentText);
        int length = objectLengthCalculator(offset);
        for (int i = offset;i<offset + length;i++)
            result.append(componentText.at(i));
    } else {
        result = componentText; //implicit component
    }

    node.variantProperty("__component_data") = result;
}

QList<RewriterView::Error> RewriterView::errors() const
{
    return m_errors;
}

void RewriterView::clearErrors()
{
    m_errors.clear();
    emit errorsChanged(m_errors);
}

void RewriterView::addErrors(const QList<RewriterView::Error> &errors)
{
    m_errors.append(errors);
    emit errorsChanged(m_errors);
}

void RewriterView::addError(const RewriterView::Error &error)
{
    m_errors.append(error);
    emit errorsChanged(m_errors);
}

QMap<ModelNode, QString> RewriterView::extractText(const QList<ModelNode> &nodes) const
{
    QmlEditor::ASTObjectTextExtractor extract(m_textModifier->text());
    QMap<ModelNode, QString> result;

    foreach (const ModelNode &node, nodes) {
        const int nodeLocation = m_positionStorage->nodeOffset(node);

        if (nodeLocation == ModelNodePositionStorage::INVALID_LOCATION)
            result.insert(node, QString::null);
        else
            result.insert(node, extract(nodeLocation));
    }

    return result;
}

int RewriterView::nodeOffset(const ModelNode &node) const
{
    return m_positionStorage->nodeOffset(node);
}

int RewriterView::nodeLength(const ModelNode &node) const
{
    ObjectLengthCalculator objectLengthCalculator(m_textModifier->text());
    return objectLengthCalculator(nodeOffset(node));
}

int RewriterView::firstDefinitionInsideOffset(const ModelNode &node) const
{
    FirstDefinitionFinder firstDefinitionFinder(m_textModifier->text());
    return firstDefinitionFinder(nodeOffset(node));
}

int RewriterView::firstDefinitionInsideLength(const ModelNode &node) const
{
    FirstDefinitionFinder firstDefinitionFinder(m_textModifier->text());
    int offset =  firstDefinitionFinder(nodeOffset(node));
    ObjectLengthCalculator objectLengthCalculator(m_textModifier->text());
    return objectLengthCalculator(offset);
}

void RewriterView::qmlTextChanged()
{
    if (m_textToModelMerger && m_textModifier) {
        const QString newQmlText = m_textModifier->text();

//        qDebug() << "qmlTextChanged:" << newQmlText;

        switch (m_differenceHandling) {
            case Validate: {
                ModelValidator differenceHandler(m_textToModelMerger.data());
                m_textToModelMerger->load(newQmlText.toUtf8(), differenceHandler);
                break;
            }

            case Amend:
            default: {
                emitCustomNotification(StartRewriterAmend);
                ModelAmender differenceHandler(m_textToModelMerger.data());
                m_textToModelMerger->load(newQmlText.toUtf8(), differenceHandler);
                emitCustomNotification(EndRewriterAmend);
                break;
            }
        }
    }
}

} //QmlDesigner
