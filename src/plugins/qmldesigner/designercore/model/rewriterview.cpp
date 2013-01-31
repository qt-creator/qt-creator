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

#include <filemanager/astobjecttextextractor.h>
#include <filemanager/objectlengthcalculator.h>
#include <filemanager/firstdefinitionfinder.h>
#include <customnotifications.h>

#include <qmljs/parser/qmljsengine_p.h>

#include "rewriterview.h"
#include "rewritingexception.h"
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

RewriterView::Error::Error(Exception *exception):
        m_type(InternalError),
        m_line(exception->line()),
        m_column(-1),
        m_description(exception->description()),
        m_url(exception->file())
{
}

RewriterView::Error::Error(const QmlJS::DiagnosticMessage &qmlError, const QUrl &document):
        m_type(ParseError),
        m_line(qmlError.loc.startLine),
        m_column(qmlError.loc.startColumn),
        m_description(qmlError.message),
        m_url(document)
{
}

RewriterView::Error::Error(const QString &shortDescription) :
    m_type(ParseError),
    m_line(1),
    m_column(0),
    m_description(shortDescription),
    m_url()
{
}


QString RewriterView::Error::toString() const
{
    QString str;

    if (m_type == ParseError)
        str += tr("Error parsing");
    else if (m_type == InternalError)
        str += tr("Internal error");

    if (url().isValid()) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');

        str += tr("\"%1\"").arg(url().toString());
    }

    if (line() != -1) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');
        str += tr("line %1").arg(line());
    }

    if (column() != -1) {
        if (!str.isEmpty())
            str += QLatin1Char(' ');

        str += tr("column %1").arg(column());
    }

    if (!str.isEmpty())
        QLatin1String(": ");
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
        transactionLevel(0),
        m_checkErrors(true)
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
    const QString qmlSource = m_textModifier->text();
    if (m_textToModelMerger->load(qmlSource, differenceHandler))
        lastCorrectQmlSource = qmlSource;
}

void RewriterView::modelAboutToBeDetached(Model * /*model*/)
{
    m_positionStorage->clear();
}

void RewriterView::nodeCreated(const ModelNode &createdNode)
{
    Q_ASSERT(textModifier());
    m_positionStorage->setNodeOffset(createdNode, ModelNodePositionStorage::INVALID_LOCATION);
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeCreated(createdNode);

    if (!isModificationGroupActive())
        applyChanges();
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
        applyChanges();
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
        applyChanges();
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
        applyChanges();
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
        applyChanges();
}

void RewriterView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeReparented(node, newPropertyParent, oldPropertyParent, propertyChange);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

void RewriterView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    foreach (const Import &import, addedImports)
        importAdded(import);

    foreach (const Import &import, removedImports)
        importRemoved(import);

}

void RewriterView::importAdded(const Import &import)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    if (import.url() == "Qt")
        foreach (const Import &import, model()->imports()) {
            if (import.url() == "QtQuick")
                return; //QtQuick magic we do not have to add an import for Qt
        }

    modelToTextMerger()->addImport(import);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::importRemoved(const Import &import)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->removeImport(import);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::fileUrlChanged(const QUrl & /*oldUrl*/, const QUrl & /*newUrl*/)
{
}

void RewriterView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeIdChanged(node, newId, oldId);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int /*oldIndex*/)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    const QList<ModelNode> nodes = listProperty.toModelNodeList();

    ModelNode trailingNode;
    int newIndex = nodes.indexOf(movedNode);
    if (newIndex + 1 < nodes.size())
        trailingNode = nodes.at(newIndex + 1);
    modelToTextMerger()->nodeSlidAround(movedNode, trailingNode);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
     Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeTypeChanged(rootModelNode(), type, majorVersion, minorVersion);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> & /* nodeList */, const QList<QVariant> & /*data */)
{
    if (identifier == StartRewriterAmend || identifier == EndRewriterAmend)
        return; // we emitted this ourselves, so just ignore these notifications.
}

void RewriterView::scriptFunctionsChanged(const ModelNode & /*node*/, const QStringList & /*scriptFunctionList*/)
{
}

void RewriterView::instancePropertyChange(const QList<QPair<ModelNode, QString> > & /*propertyList*/)
{
}

void RewriterView::instancesCompleted(const QVector<ModelNode> & /*completedNodeList*/)
{
}

void RewriterView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> & /* informationChangeHash */)
{

}

void RewriterView::instancesRenderImageChanged(const QVector<ModelNode> & /*nodeList*/)
{

}

void RewriterView::instancesPreviewImageChanged(const QVector<ModelNode> & /*nodeList*/)
{

}

void RewriterView::instancesChildrenChanged(const QVector<ModelNode> & /*nodeList*/)
{

}

void RewriterView::instancesToken(const QString &/*tokenName*/, int /*tokenNumber*/, const QVector<ModelNode> &/*nodeVector*/)
{

}

void RewriterView::nodeSourceChanged(const ModelNode &, const QString & /*newNodeSource*/)
{

}

void RewriterView::rewriterBeginTransaction()
{
    transactionLevel++;
    setModificationGroupActive(true);
}

void RewriterView::rewriterEndTransaction()
{
    transactionLevel--;
    Q_ASSERT(transactionLevel >= 0);
    if (transactionLevel == 0)
    {
        setModificationGroupActive(false);
        applyModificationGroupChanges();
    }
}

void RewriterView::actualStateChanged(const ModelNode & /*node*/)
{
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

QString RewriterView::textModifierContent() const
{
    if (textModifier())
        return textModifier()->text();

    return QString();
}

void RewriterView::reactivateTextMofifierChangeSignals()
{
    textModifier()->reactivateChangeSignals();
}

void RewriterView::deactivateTextMofifierChangeSignals()
{
    textModifier()->deactivateChangeSignals();
}

void RewriterView::applyModificationGroupChanges()
{
    Q_ASSERT(transactionLevel == 0);
    applyChanges();
}

void RewriterView::applyChanges()
{
    if (modelToTextMerger()->hasNoPendingChanges())
        return; // quick exit: nothing to be done.

    clearErrors();

    if (inErrorState()) {
        const QString content = textModifierContent();
        qDebug() << "RewriterView::applyChanges() got called while in error state. Will do a quick-exit now.";
        qDebug() << "Content:" << content;
        throw RewritingException(__LINE__, __FUNCTION__, __FILE__, "RewriterView::applyChanges() already in error state", content);
    }

    m_differenceHandling = Validate;

    try {
        modelToTextMerger()->applyChanges();
        if (!errors().isEmpty())
            enterErrorState(errors().first().description());
    } catch (Exception &e) {
        const QString content = textModifierContent();
        qDebug() << "RewriterException:" << m_rewritingErrorMessage;
        qDebug() << "Content:" << content;
        enterErrorState(e.description());
    }

    m_differenceHandling = Amend;

    if (inErrorState()) {
        const QString content = textModifierContent();
        qDebug() << "RewriterException:" << m_rewritingErrorMessage;
        qDebug() << "Content:" << content;
        if (!errors().isEmpty())
            qDebug() << "Error:" << errors().first().description();
        throw RewritingException(__LINE__, __FUNCTION__, __FILE__, m_rewritingErrorMessage, content);
    }
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

void RewriterView::setErrors(const QList<RewriterView::Error> &errors)
{
    m_errors = errors;
    emit errorsChanged(m_errors);
}

void RewriterView::addError(const RewriterView::Error &error)
{
    m_errors.append(error);
    emit errorsChanged(m_errors);
}

void RewriterView::enterErrorState(const QString &errorMessage)
{
    m_rewritingErrorMessage = errorMessage;
}

void RewriterView::resetToLastCorrectQml()
{
    m_textModifier->textDocument()->undo();
    m_textModifier->textDocument()->clearUndoRedoStacks(QTextDocument::RedoStack);
    ModelAmender differenceHandler(m_textToModelMerger.data());
    m_textToModelMerger->load(m_textModifier->text().toUtf8(), differenceHandler);

    leaveErrorState();
}

QMap<ModelNode, QString> RewriterView::extractText(const QList<ModelNode> &nodes) const
{
    QmlDesigner::ASTObjectTextExtractor extract(m_textModifier->text());
    QMap<ModelNode, QString> result;

    foreach (const ModelNode &node, nodes) {
        const int nodeLocation = m_positionStorage->nodeOffset(node);

        if (nodeLocation == ModelNodePositionStorage::INVALID_LOCATION)
            result.insert(node, QString());
        else
            result.insert(node, extract(nodeLocation));
    }

    return result;
}

int RewriterView::nodeOffset(const ModelNode &node) const
{
    return m_positionStorage->nodeOffset(node);
}

/**
 * \return the length of the node's text, or -1 if it wasn't found or if an error
 *         occurred.
 */
int RewriterView::nodeLength(const ModelNode &node) const
{
    ObjectLengthCalculator objectLengthCalculator;
    unsigned length;
    if (objectLengthCalculator(m_textModifier->text(), nodeOffset(node), length))
        return (int) length;
    else
        return -1;
}

int RewriterView::firstDefinitionInsideOffset(const ModelNode &node) const
{
    FirstDefinitionFinder firstDefinitionFinder(m_textModifier->text());
    return firstDefinitionFinder(nodeOffset(node));
}

int RewriterView::firstDefinitionInsideLength(const ModelNode &node) const
{
    FirstDefinitionFinder firstDefinitionFinder(m_textModifier->text());
    const int offset =  firstDefinitionFinder(nodeOffset(node));

    ObjectLengthCalculator objectLengthCalculator;
    unsigned length;
    if (objectLengthCalculator(m_textModifier->text(), offset, length))
        return length;
    else
        return -1;
}

bool RewriterView::modificationGroupActive()
{
    return m_modificationGroupActive;
}

static bool isInNodeDefinition(int nodeTextOffset, int nodeTextLength, int cursorPosition)
{
    return (nodeTextOffset <= cursorPosition) && (nodeTextOffset + nodeTextLength > cursorPosition);
}

ModelNode RewriterView::nodeAtTextCursorPosition(int cursorPosition) const
{
    const QList<ModelNode> allNodes = allModelNodes();

    ModelNode nearestNode;
    int nearestNodeTextOffset = -1;

    foreach (const ModelNode &currentNode, allNodes) {
        const int nodeTextOffset = nodeOffset(currentNode);
        const int nodeTextLength = nodeLength(currentNode);
        if (isInNodeDefinition(nodeTextOffset, nodeTextLength, cursorPosition)
            && (nodeTextOffset > nearestNodeTextOffset)) {
            nearestNode = currentNode;
            nearestNodeTextOffset = nodeTextOffset;
        }
    }

    return nearestNode;
}

bool RewriterView::renameId(const QString& oldId, const QString& newId)
{
    if (textModifier())
        return textModifier()->renameId(oldId, newId);

    return false;
}

const QmlJS::ScopeChain *RewriterView::scopeChain() const
{
    return textToModelMerger()->scopeChain();
}

const QmlJS::Document *RewriterView::document() const
{
    return textToModelMerger()->document();
}

static inline QString getUrlFromType(const QString& typeName)
{
    QStringList nameComponents = typeName.split('.');
    QString result;

    for (int i = 0; i < (nameComponents.count() - 1); i++) {
        result += nameComponents.at(i);
    }

    return result;
}

QString RewriterView::convertTypeToImportAlias(const QString &type) const
{
    QString url;
    QString simplifiedType = type;
    if (type.contains('.')) {
        QStringList nameComponents = type.split('.');
        url = getUrlFromType(type);
        simplifiedType = nameComponents.last();
    }

    QString alias;
    if (!url.isEmpty()) {
        foreach (const Import &import, model()->imports()) {
            if (import.url() == url) {
                alias = import.alias();
                break;
            }
            if (import.file() == url) {
                alias = import.alias();
                break;
            }
        }
    }

    QString result;

    if (!alias.isEmpty())
        result = alias + '.';

    result += simplifiedType;

    return result;
}

QString RewriterView::pathForImport(const Import &import)
{
    if (scopeChain() && scopeChain()->context() && document()) {
        const QString importStr = import.isFileImport() ? import.file() : import.url();
        const QmlJS::Imports *imports = scopeChain()->context()->imports(document());

        QmlJS::ImportInfo importInfo;

        foreach (QmlJS::Import qmljsImport, imports->all()) {
            if (qmljsImport.info.name() == importStr)
                importInfo = qmljsImport.info;
        }
        const QString importPath = importInfo.path();
        return importPath;
    }

    return QString();
}

QWidget *RewriterView::widget()
{
    return 0;
}

void RewriterView::qmlTextChanged()
{
    if (inErrorState())
        return;

    if (m_textToModelMerger && m_textModifier) {
        const QString newQmlText = m_textModifier->text();

#if 0
        qDebug() << Q_FUNC_INFO;
        qDebug() << "old:" << lastCorrectQmlSource;
        qDebug() << "new:" << newQmlText;
#endif

        switch (m_differenceHandling) {
            case Validate: {
                ModelValidator differenceHandler(m_textToModelMerger.data());
                if (m_textToModelMerger->load(newQmlText.toUtf8(), differenceHandler))
                    lastCorrectQmlSource = newQmlText;
                break;
            }

            case Amend:
            default: {
                emitCustomNotification(StartRewriterAmend);
                ModelAmender differenceHandler(m_textToModelMerger.data());
                if (m_textToModelMerger->load(newQmlText, differenceHandler))
                    lastCorrectQmlSource = newQmlText;
                emitCustomNotification(EndRewriterAmend);
                break;
            }
        }
    }
}

void RewriterView::delayedSetup()
{
    if (m_textToModelMerger)
        m_textToModelMerger->delayedSetup();
}

} //QmlDesigner
