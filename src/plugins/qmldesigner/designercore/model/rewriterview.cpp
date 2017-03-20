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

#include "rewriterview.h"

#include "texttomodelmerger.h"
#include "modeltotextmerger.h"

#include <variantproperty.h>
#include <bindingproperty.h>
#include <rewritingexception.h>
#include <signalhandlerproperty.h>
#include <filemanager/astobjecttextextractor.h>
#include <filemanager/objectlengthcalculator.h>
#include <filemanager/firstdefinitionfinder.h>
#include <customnotifications.h>
#include <modelnodepositionstorage.h>
#include <modelnode.h>
#include <nodeproperty.h>

#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

using namespace QmlDesigner::Internal;

namespace QmlDesigner {

RewriterView::RewriterView(DifferenceHandling differenceHandling, QObject *parent):
        AbstractView(parent),
        m_differenceHandling(differenceHandling),
        m_positionStorage(new ModelNodePositionStorage),
        m_modelToTextMerger(new Internal::ModelToTextMerger(this)),
        m_textToModelMerger(new Internal::TextToModelMerger(this))
{
    m_amendTimer.setSingleShot(true);
    connect(&m_amendTimer, &QTimer::timeout, this, &RewriterView::amendQmlText);
}

RewriterView::~RewriterView()
{
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
    if (model && model->textModifier())
        setTextModifier(model->textModifier());

    AbstractView::modelAttached(model);

    ModelAmender differenceHandler(m_textToModelMerger.data());
    const QString qmlSource = m_textModifier->text();
    if (m_textToModelMerger->load(qmlSource, differenceHandler))
        m_lastCorrectQmlSource = qmlSource;

    if (!(m_errors.isEmpty() && m_warnings.isEmpty()))
        notifyErrorsAndWarnings(m_errors);
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

void RewriterView::nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeRemoved(removedNode, parentProperty, propertyChange);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;


    foreach (const AbstractProperty &property, propertyList) {
        if (property.isDefaultProperty() && property.isNodeListProperty()) {
            m_removeDefaultPropertyTransaction = beginRewriterTransaction(QByteArrayLiteral("RewriterView::propertiesAboutToBeRemoved"));

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

void RewriterView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList, AbstractView::PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    QList<AbstractProperty> usefulPropertyList;
    foreach (const SignalHandlerProperty &property, propertyList)
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

    if (import.url() == QLatin1String("Qt"))
        foreach (const Import &import, model()->imports()) {
            if (import.url() == QLatin1String("QtQuick"))
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

    ModelNode trailingNode;
    int newIndex = listProperty.indexOf(movedNode);
    if (newIndex + 1 < listProperty.count())
        trailingNode = listProperty.at(newIndex + 1);
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

void RewriterView::nodeTypeChanged(const ModelNode &node, const TypeName &type, int majorVersion, int minorVersion)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeTypeChanged(node, QString::fromLatin1(type), majorVersion, minorVersion);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> & /* nodeList */, const QList<QVariant> & /*data */)
{
    if (identifier == StartRewriterAmend || identifier == EndRewriterAmend)
        return; // we emitted this ourselves, so just ignore these notifications.
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
        disconnect(m_textModifier, &TextModifier::textChanged, this, &RewriterView::qmlTextChanged);

    m_textModifier = textModifier;

    if (m_textModifier)
        connect(m_textModifier, &TextModifier::textChanged, this, &RewriterView::qmlTextChanged);
}

QString RewriterView::textModifierContent() const
{
    if (textModifier())
        return textModifier()->text();

    return QString();
}

void RewriterView::reactivateTextMofifierChangeSignals()
{
    if (textModifier())
        textModifier()->reactivateChangeSignals();
}

void RewriterView::deactivateTextMofifierChangeSignals()
{
    if (textModifier())
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

    clearErrorAndWarnings();

    if (inErrorState()) {
        const QString content = textModifierContent();
        qDebug().noquote() << "RewriterView::applyChanges() got called while in error state. Will do a quick-exit now.";
        qDebug().noquote() << "Content: " << content;
        throw RewritingException(__LINE__, __FUNCTION__, __FILE__, "RewriterView::applyChanges() already in error state", content);
    }

    m_differenceHandling = Validate;

    try {
        modelToTextMerger()->applyChanges();
        if (!errors().isEmpty())
            enterErrorState(errors().first().description());
    } catch (const Exception &e) {
        const QString content = textModifierContent();
        qDebug().noquote() << "RewriterException:" << m_rewritingErrorMessage;
        qDebug().noquote() << "Content: " << qPrintable(content);
        enterErrorState(e.description());
    }

    m_differenceHandling = Amend;

    if (inErrorState()) {
        const QString content = textModifierContent();
        qDebug().noquote() << "RewriterException: " << m_rewritingErrorMessage;
        qDebug().noquote() << "Content: " << content;
        if (!errors().isEmpty())
            qDebug().noquote() << "Error:" << errors().first().description();
        throw RewritingException(__LINE__, __FUNCTION__, __FILE__, qPrintable(m_rewritingErrorMessage), content);
    }
}

void RewriterView::amendQmlText()
{
    emitCustomNotification(StartRewriterAmend);

    const QString newQmlText = m_textModifier->text();

    ModelAmender differenceHandler(m_textToModelMerger.data());
    if (m_textToModelMerger->load(newQmlText, differenceHandler))
        m_lastCorrectQmlSource = newQmlText;
    emitCustomNotification(EndRewriterAmend);
}

void RewriterView::notifyErrorsAndWarnings(const QList<DocumentMessage> &errors)
{
    if (m_setWidgetStatusCallback)
        m_setWidgetStatusCallback(errors.isEmpty());

    emitDocumentMessage(errors, m_warnings);
}

Internal::ModelNodePositionStorage *RewriterView::positionStorage() const
{
    return m_positionStorage.data();
}

QList<DocumentMessage> RewriterView::warnings() const
{
    return m_warnings;
}

QList<DocumentMessage> RewriterView::errors() const
{
    return m_errors;
}

void RewriterView::clearErrorAndWarnings()
{
    m_errors.clear();
    m_warnings.clear();
    notifyErrorsAndWarnings(m_errors);
}

void RewriterView::setWarnings(const QList<DocumentMessage> &warnings)
{
    m_warnings = warnings;
    notifyErrorsAndWarnings(m_errors);
}

void RewriterView::setErrors(const QList<DocumentMessage> &errors)
{
    m_errors = errors;
    notifyErrorsAndWarnings(m_errors);
}

void RewriterView::addError(const DocumentMessage &error)
{
    m_errors.append(error);
    notifyErrorsAndWarnings(m_errors);
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
    m_textToModelMerger->load(m_textModifier->text(), differenceHandler);

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

ModelNode RewriterView::nodeAtTextCursorPositionRekursive(const ModelNode &root, int cursorPosition) const
{
    ModelNode node = root;

    int lastOffset = -1;

    bool sorted = true;

    if (!root.nodeProperties().isEmpty())
        sorted = false;

    foreach (const ModelNode &currentNode, node.directSubModelNodes()) {
        const int offset = nodeOffset(currentNode);

        if (offset < cursorPosition && offset > lastOffset) {
            node = nodeAtTextCursorPositionRekursive(currentNode, cursorPosition);
            lastOffset = offset;
        } else {
            if (sorted)
                break;
        }
    }

    const int nodeTextLength = nodeLength(node);
    const int nodeTextOffset = nodeOffset(node);

    if (nodeTextLength < 0)
        return ModelNode();

    if (isInNodeDefinition(nodeTextOffset, nodeTextLength, cursorPosition))
        return node;

    return root;
}

ModelNode RewriterView::nodeAtTextCursorPosition(int cursorPosition) const
{
    return nodeAtTextCursorPositionRekursive(rootModelNode(), cursorPosition);
}

bool RewriterView::nodeContainsCursor(const ModelNode &node, int cursorPosition) const
{
    const int nodeTextLength = nodeLength(node);
    const int nodeTextOffset = nodeOffset(node);

    if (isInNodeDefinition(nodeTextOffset, nodeTextLength, cursorPosition))
        return true;

    return false;
}

bool RewriterView::renameId(const QString& oldId, const QString& newId)
{
    if (textModifier()) {
        PropertyName propertyName = oldId.toUtf8();

        bool hasAliasExport = rootModelNode().isValid()
                && rootModelNode().hasBindingProperty(propertyName)
                && rootModelNode().bindingProperty(propertyName).isAliasExport();

        bool instant = m_instantQmlTextUpdate;
        m_instantQmlTextUpdate = true;

        bool refactoring =  textModifier()->renameId(oldId, newId);

        m_instantQmlTextUpdate = instant;

        if (refactoring && hasAliasExport) { //Keep export alias properties
            rootModelNode().removeProperty(propertyName);
            PropertyName newPropertyName = newId.toUtf8();
            rootModelNode().bindingProperty(newPropertyName).setDynamicTypeNameAndExpression("alias", QString::fromUtf8(newPropertyName));
        }
        return refactoring;
    }

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

        foreach (const QmlJS::Import &qmljsImport, imports->all()) {
            if (qmljsImport.info.name() == importStr)
                importInfo = qmljsImport.info;
        }
        const QString importPath = importInfo.path();
        return importPath;
    }

    return QString();
}

QStringList RewriterView::importDirectories() const
{
    return m_textToModelMerger->vContext().paths;
}

QSet<QPair<QString, QString> > RewriterView::qrcMapping() const
{
    return m_textToModelMerger->qrcMapping();
}

void RewriterView::moveToComponent(const ModelNode &modelNode)
{
    int offset = nodeOffset(modelNode);

    bool instant = m_instantQmlTextUpdate;
    m_instantQmlTextUpdate = true;

    textModifier()->moveToComponent(offset);

    m_instantQmlTextUpdate = instant;
}

QStringList RewriterView::autoComplete(const QString &text, int pos, bool explicitComplete)
{
    QTextDocument textDocument;
    textDocument.setPlainText(text);

    QStringList list = textModifier()->autoComplete(&textDocument, pos, explicitComplete);
    list.removeDuplicates();

    return list;
}

QList<CppTypeData> RewriterView::getCppTypes()
{
    QList<CppTypeData> cppDataList;
    for (const QmlJS::ModelManagerInterface::CppData &cppData : QmlJS::ModelManagerInterface::instance()->cppData().values())
        for (const LanguageUtils::FakeMetaObject::ConstPtr &fakeMetaObject : cppData.exportedTypes) {
            for (const LanguageUtils::FakeMetaObject::Export &exportItem : fakeMetaObject->exports()) {

            CppTypeData cppData;
            cppData.cppClassName = fakeMetaObject->className();
            cppData.typeName = exportItem.type;
            cppData.importUrl = exportItem.package;
            cppData.versionString = exportItem.version.toString();
            cppData.superClassName = fakeMetaObject->superclassName();
            cppData.isSingleton = fakeMetaObject->isSingleton();

            if (cppData.importUrl != "<cpp>") //ignore pure unregistered cpp types
                cppDataList.append(cppData);
            }
        }

    return cppDataList;
}

void RewriterView::setWidgetStatusCallback(std::function<void (bool)> setWidgetStatusCallback)
{
    m_setWidgetStatusCallback = setWidgetStatusCallback;
}

void RewriterView::qmlTextChanged()
{
    getCppTypes();
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
            if (m_textToModelMerger->load(newQmlText, differenceHandler))
                m_lastCorrectQmlSource = newQmlText;
            break;
        }

        case Amend: {
            if (m_instantQmlTextUpdate)
                amendQmlText();
            else
#ifndef QMLDESIGNER_TEST
                m_amendTimer.start(400);
#else
                /*Keep test synchronous*/
                amendQmlText();
#endif
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
