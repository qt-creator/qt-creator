// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewriterview.h"

#include "model_p.h"
#include "modeltotextmerger.h"
#include "texttomodelmerger.h"

#include <bindingproperty.h>
#include <customnotifications.h>
#include <designersettings.h>
#include <externaldependenciesinterface.h>
#include <filemanager/astobjecttextextractor.h>
#include <filemanager/firstdefinitionfinder.h>
#include <filemanager/objectlengthcalculator.h>
#include <modelnode.h>
#include <modelnodepositionstorage.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <projectstorage/projectstorage.h>
#include <rewritingexception.h>
#include <signalhandlerproperty.h>
#include <variantproperty.h>

#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljssimplereader.h>

#include <utils/algorithm.h>
#include <utils/changeset.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <utility>
#include <vector>

using namespace QmlDesigner::Internal;
using namespace Qt::StringLiterals;

namespace QmlDesigner {

constexpr QStringView annotationsStart{u"/*##^##"};
constexpr QStringView annotationsEnd{u"##^##*/"};

bool debugQmlPuppet(const DesignerSettings &settings)
{
    const QString debugPuppet = settings.value(DesignerSettingsKey::DEBUG_PUPPET).toString();
    return !debugPuppet.isEmpty();
}

RewriterView::RewriterView(ExternalDependenciesInterface &externalDependencies,
                           DifferenceHandling differenceHandling,
                           InstantQmlTextUpdate instantQmlTextUpdate)
    : AbstractView{externalDependencies}
    , m_differenceHandling(differenceHandling)
    , m_positionStorage(std::make_unique<ModelNodePositionStorage>())
    , m_modelToTextMerger(std::make_unique<Internal::ModelToTextMerger>(this))
    , m_textToModelMerger(std::make_unique<Internal::TextToModelMerger>(this))
    , m_instantQmlTextUpdate(instantQmlTextUpdate)
{
    setKind(Kind::Rewriter);

    m_amendTimer.setSingleShot(true);

    if (m_instantQmlTextUpdate == InstantQmlTextUpdate::No) {
        m_amendTimer.setInterval(800);
        connect(&m_amendTimer, &QTimer::timeout, this, &RewriterView::amendQmlText);
    }

#ifndef QDS_USE_PROJECTSTORAGE
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    connect(modelManager,
            &QmlJS::ModelManagerInterface::libraryInfoUpdated,
            this,
            &RewriterView::handleLibraryInfoUpdate,
            Qt::QueuedConnection);
    connect(modelManager,
            &QmlJS::ModelManagerInterface::projectInfoUpdated,
            this,
            &RewriterView::handleProjectUpdate,
            Qt::DirectConnection);
    connect(this,
            &RewriterView::modelInterfaceProjectUpdated,
            this,
            &RewriterView::handleLibraryInfoUpdate,
            Qt::QueuedConnection);
#endif
}

RewriterView::~RewriterView() = default;

Internal::ModelToTextMerger *RewriterView::modelToTextMerger() const
{
    return m_modelToTextMerger.get();
}

Internal::TextToModelMerger *RewriterView::textToModelMerger() const
{
    return m_textToModelMerger.get();
}

void RewriterView::modelAttached(Model *model)
{
    QTC_ASSERT(m_textModifier, return);
    m_modelAttachPending = false;

    AbstractView::modelAttached(model);

    ModelAmender differenceHandler(m_textToModelMerger.get());
    const QString qmlSource = m_textModifier->text();
    if (m_textToModelMerger->load(qmlSource, differenceHandler))
        m_lastCorrectQmlSource = qmlSource;

    if (!(m_errors.isEmpty() && m_warnings.isEmpty()))
        notifyErrorsAndWarnings(m_errors);

    if (m_instantQmlTextUpdate == InstantQmlTextUpdate::Yes) {
        restoreAuxiliaryData();
    } else if (hasIncompleteTypeInformation()) {
        m_modelAttachPending = true;
        QTimer::singleShot(1000, this, [this, model]() {
            modelAttached(model);
            restoreAuxiliaryData();
        });
    }
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

void RewriterView::nodeRemoved(const ModelNode &removedNode,
                               const NodeAbstractProperty &parentProperty,
                               PropertyChangeFlags propertyChange)
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

    for (const AbstractProperty &property : propertyList) {
        if (!property.isDefaultProperty())
            continue;

        if (!m_removeDefaultPropertyTransaction.isValid()) {
            m_removeDefaultPropertyTransaction = beginRewriterTransaction(
                QByteArrayLiteral("RewriterView::propertiesAboutToBeRemoved"));
        }

        if (property.isNodeListProperty()) {
            const auto nodeList = property.toNodeListProperty().toModelNodeList();
            for (const ModelNode &node : nodeList) {
                modelToTextMerger()->nodeRemoved(node,
                                                 property.toNodeAbstractProperty(),
                                                 AbstractView::NoAdditionalChanges);
            }
        } else if (property.isBindingProperty() || property.isVariantProperty()
                   || property.isNodeProperty()) {
            // Default property that has actual binding/value should be removed.
            // We need to do it here in propertiesAboutToBeRemoved, because
            // type is no longer determinable after property is removed from the model.
            modelToTextMerger()->propertiesRemoved({property});
        }
    }
}

void RewriterView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
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

void RewriterView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                            PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    QList<AbstractProperty> usefulPropertyList;
    for (const VariantProperty &property : propertyList)
        usefulPropertyList.append(property);

    modelToTextMerger()->propertiesChanged(usefulPropertyList, propertyChange);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                            PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    QList<AbstractProperty> usefulPropertyList;
    for (const BindingProperty &property : propertyList)
        usefulPropertyList.append(property);

    modelToTextMerger()->propertiesChanged(usefulPropertyList, propertyChange);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList,
                                                  AbstractView::PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    QList<AbstractProperty> usefulPropertyList;
    for (const SignalHandlerProperty &property : propertyList)
        usefulPropertyList.append(property);

    modelToTextMerger()->propertiesChanged(usefulPropertyList, propertyChange);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::signalDeclarationPropertiesChanged(
    const QVector<SignalDeclarationProperty> &propertyList, PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    QList<AbstractProperty> usefulPropertyList;
    for (const SignalDeclarationProperty &property : propertyList)
        usefulPropertyList.append(property);

    modelToTextMerger()->propertiesChanged(usefulPropertyList, propertyChange);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::nodeReparented(const ModelNode &node,
                                  const NodeAbstractProperty &newPropertyParent,
                                  const NodeAbstractProperty &oldPropertyParent,
                                  AbstractView::PropertyChangeFlags propertyChange)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeReparented(node, newPropertyParent, oldPropertyParent, propertyChange);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::importsChanged(const Imports &addedImports, const Imports &removedImports)
{
    importsAdded(addedImports);
    importsRemoved(removedImports);
}

void RewriterView::importsAdded(const Imports &imports)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->addImports(imports);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::importsRemoved(const Imports &imports)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->removeImports(imports);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeIdChanged(node, newId, oldId);

    if (!isModificationGroupActive())
        applyChanges();
}

void RewriterView::nodeOrderChanged(const NodeListProperty &listProperty,
                                    const ModelNode &movedNode,
                                    int /*oldIndex*/)
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

void RewriterView::nodeOrderChanged(const NodeListProperty &listProperty)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    auto modelNodes = listProperty.directSubNodes();

    for (const ModelNode &movedNode : modelNodes)
        modelToTextMerger()->nodeSlidAround(movedNode, ModelNode{});

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

void RewriterView::nodeTypeChanged(const ModelNode &node,
                                   const TypeName &type,
                                   int majorVersion,
                                   int minorVersion)
{
    Q_ASSERT(textModifier());
    if (textToModelMerger()->isActive())
        return;

    modelToTextMerger()->nodeTypeChanged(node, QString::fromLatin1(type), majorVersion, minorVersion);

    if (!isModificationGroupActive())
        applyChanges();
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
    if (transactionLevel == 0) {
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

void RewriterView::reactivateTextModifierChangeSignals()
{
    if (textModifier())
        textModifier()->reactivateChangeSignals();
}

void RewriterView::deactivateTextModifierChangeSignals()
{
    if (textModifier())
        textModifier()->deactivateChangeSignals();
}

void RewriterView::auxiliaryDataChanged(const ModelNode &, AuxiliaryDataKeyView key, const QVariant &)
{
    if (m_restoringAuxData || !m_textModifier)
        return;

    if (key.type == AuxiliaryDataType::Document)
        m_textModifier->textDocument()->setModified(true);
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
        qDebug().noquote() << "RewriterView::applyChanges() got called while in error state. Will "
                              "do a quick-exit now.";
        qDebug().noquote() << "Content: " << content;
        throw RewritingException(__LINE__,
                                 __FUNCTION__,
                                 __FILE__,
                                 "RewriterView::applyChanges() already in error state",
                                 content);
    }

    m_differenceHandling = Validate;

    try {
        modelToTextMerger()->applyChanges();
        if (!errors().isEmpty())
            enterErrorState(errors().constFirst().description());
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
            qDebug().noquote() << "Error:" << errors().constFirst().description();
        throw RewritingException(__LINE__,
                                 __FUNCTION__,
                                 __FILE__,
                                 qPrintable(m_rewritingErrorMessage),
                                 content);
    }
}

void RewriterView::amendQmlText()
{
    if (!model()->rewriterView())
        return;
    QTC_ASSERT(m_textModifier, return);

    emitCustomNotification(StartRewriterAmend);
    const QString newQmlText = m_textModifier->text();

    ModelAmender differenceHandler(m_textToModelMerger.get());
    if (m_textToModelMerger->load(newQmlText, differenceHandler))
        m_lastCorrectQmlSource = newQmlText;
    emitCustomNotification(EndRewriterAmend);
}

void RewriterView::notifyErrorsAndWarnings(const QList<DocumentMessage> &errors)
{
    if (m_setWidgetStatusCallback)
        m_setWidgetStatusCallback(errors.isEmpty());

    if (isAttached())
        model()->emitDocumentMessage(errors, m_warnings);
}

static QString replaceIllegalPropertyNameChars(const QString &str)
{
    QString ret = str;

    ret.replace("@", "__AT__");

    return ret;
}

static bool idIsQmlKeyWord(QStringView id)
{
    static constexpr std::u16string_view keywords[] = {
        u"as",     u"break", u"case",       u"catch",   u"continue", u"debugger", u"default",
        u"delete", u"do",    u"else",       u"finally", u"for",      u"function", u"if",
        u"import", u"in",    u"instanceof", u"new",     u"return",   u"switch",   u"this",
        u"throw",  u"try",   u"typeof",     u"var",     u"void",     u"while",    u"with"};

    return Utils::contains(keywords, std::u16string_view{id.utf16(), Utils::usize(id)});
}

QString RewriterView::auxiliaryDataAsQML() const
{
    bool hasAuxData = false;

    setupCanonicalHashes();

    QString str = "Designer {\n    ";

    QTC_ASSERT(!m_canonicalIntModelNode.isEmpty(), return {});

    int columnCount = 0;

    static const QRegularExpression safeName(R"(^[a-z][a-zA-Z0-9]*$)");

    for (const auto &node : allModelNodes()) {
        auto data = node.auxiliaryData(AuxiliaryDataType::Document);
        if (!data.empty()) {
            if (columnCount > 80) {
                str += "\n";
                columnCount = 0;
            }
            const int startLen = str.length();
            str += "D{";
            str += "i:";

            str += QString::number(m_canonicalModelNodeInt.value(node));
            str += ";";

            std::sort(data.begin(), data.end(), [](const auto &first, const auto &second) {
                return first.first < second.first;
            });

            for (const auto &[keyUtf8, value] : data) {
                auto key = QString::fromUtf8(keyUtf8);
                if (idIsQmlKeyWord(key))
                    continue;

                if (!key.contains(safeName))
                    continue;
                hasAuxData = true;
                QString strValue = value.toString();

                const int metaType = value.typeId();

                if (metaType == QMetaType::QString || metaType == QMetaType::QColor) {
                    strValue.replace("\\"_L1, "\\\\"_L1);
                    strValue.replace("\""_L1, "\\\""_L1);
                    strValue.replace("\t"_L1, "\\t"_L1);
                    strValue.replace("\r"_L1, "\\r"_L1);
                    strValue.replace("\n"_L1, "\\n"_L1);
                    strValue.replace("*/"_L1, "*\\/"_L1);

                    strValue = "\"" + strValue + "\"";
                }

                if (!strValue.isEmpty()) {
                    str += replaceIllegalPropertyNameChars(key) + ":";
                    str += strValue;
                    str += ";";
                }
            }

            if (str.endsWith(';'))
                str.chop(1);

            str += "}";
            columnCount += str.length() - startLen;
        }
    }

    str += "\n}\n";

    if (hasAuxData)
        return str;

    return {};
}

ModelNode RewriterView::getNodeForCanonicalIndex(int index)
{
    return m_canonicalIntModelNode.value(index);
}

void RewriterView::setAllowComponentRoot(bool allow)
{
    m_allowComponentRoot = allow;
}

bool RewriterView::allowComponentRoot() const
{
    return m_allowComponentRoot;
}

void RewriterView::resetPossibleImports()
{
    m_textToModelMerger->clearPossibleImportKeys();
}

bool RewriterView::possibleImportsEnabled() const
{
    return m_possibleImportsEnabled;
}

void RewriterView::setPossibleImportsEnabled(bool b)
{
    m_possibleImportsEnabled = b;
}

void RewriterView::forceAmend()
{
    m_amendTimer.stop();
    amendQmlText();
}

Internal::ModelNodePositionStorage *RewriterView::positionStorage() const
{
    return m_positionStorage.get();
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

void RewriterView::setIncompleteTypeInformation(bool b)
{
    m_hasIncompleteTypeInformation = b;
}

bool RewriterView::hasIncompleteTypeInformation() const
{
    return m_hasIncompleteTypeInformation;
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
    QTC_ASSERT(m_textModifier, return);
    m_textModifier->textDocument()->undo();
    m_textModifier->textDocument()->clearUndoRedoStacks(QTextDocument::RedoStack);
    ModelAmender differenceHandler(m_textToModelMerger.get());
    Internal::WriteLocker::unlock(model());
    m_textToModelMerger->load(m_textModifier->text(), differenceHandler);
    Internal::WriteLocker::lock(model());

    leaveErrorState();
}

QMap<ModelNode, QString> RewriterView::extractText(const QList<ModelNode> &nodes) const
{
    QTC_ASSERT(m_textModifier, return {});
    QmlDesigner::ASTObjectTextExtractor extract(m_textModifier->text());
    QMap<ModelNode, QString> result;

    for (const ModelNode &node : nodes) {
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
    QTC_ASSERT(m_textModifier, return -1);
    ObjectLengthCalculator objectLengthCalculator;
    unsigned length;
    if (objectLengthCalculator(m_textModifier->text(), nodeOffset(node), length))
        return (int) length;
    else
        return -1;
}

int RewriterView::firstDefinitionInsideOffset(const ModelNode &node) const
{
    QTC_ASSERT(m_textModifier, return -1);
    FirstDefinitionFinder firstDefinitionFinder(m_textModifier->text());
    return firstDefinitionFinder(nodeOffset(node));
}

int RewriterView::firstDefinitionInsideLength(const ModelNode &node) const
{
    QTC_ASSERT(m_textModifier, return -1);
    FirstDefinitionFinder firstDefinitionFinder(m_textModifier->text());
    const int offset = firstDefinitionFinder(nodeOffset(node));

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

int findEvenClosingBrace(const QStringView &string)
{
    int count = 0;
    int index = 0;

    for (auto currentChar : string) {
        if (currentChar == '{')
            count++;

        if (currentChar == '}') {
            if (count == 1)
                return index;
            else
                count--;
        }
        index++;
    }
    return index;
}

ModelNode RewriterView::nodeAtTextCursorPositionHelper(const ModelNode &root, int cursorPosition) const
{
    QTC_ASSERT(m_textModifier, return {});

    using myPair = std::pair<ModelNode, int>;
    std::vector<myPair> data;

    for (const ModelNode &node : allModelNodes()) {
        int offset = nodeOffset(node);
        if (offset > 0)
            data.emplace_back(std::make_pair(node, offset));
    }

    std::sort(data.begin(), data.end(), [](myPair a, myPair b) { return a.second < b.second; });

    ModelNode lastNode = root;

    for (const myPair &pair : data) {
        ModelNode node = pair.first;

        const int textLength = m_textModifier->text().length();
        const int nodeTextOffset = std::min(nodeOffset(node), textLength);

        const int nodeTextLength = findEvenClosingBrace(m_textModifier->text().sliced(nodeTextOffset))
                                   - 1;

        if (isInNodeDefinition(nodeTextOffset, nodeTextLength, cursorPosition))
            lastNode = node;
        else if (nodeTextOffset > cursorPosition)
            break;
    }

    return lastNode;
}

void RewriterView::setupCanonicalHashes() const
{
    m_canonicalIntModelNode.clear();
    m_canonicalModelNodeInt.clear();

    using myPair = std::pair<ModelNode, int>;
    std::vector<myPair> data;

    for (const ModelNode &node : allModelNodes()) {
        int offset = nodeOffset(node);
        if (offset > 0)
            data.emplace_back(std::make_pair(node, offset));
    }

    std::sort(data.begin(), data.end(), [](myPair a, myPair b) { return a.second < b.second; });

    int i = 0;
    for (const myPair &pair : data) {
        m_canonicalIntModelNode.insert(i, pair.first);
        m_canonicalModelNodeInt.insert(pair.first, i);
        ++i;
    }
}

#ifndef QDS_USE_PROJECTSTORAGE
void RewriterView::handleLibraryInfoUpdate()
{
    // Trigger dummy amend to reload document when library info changes
    if (isAttached() && !m_modelAttachPending
        && !debugQmlPuppet(externalDependencies().designerSettings()))
        m_amendTimer.start();

    emitCustomNotification(UpdateItemlibrary);
}

void RewriterView::handleProjectUpdate()
{
    emit modelInterfaceProjectUpdated();
}
#endif

ModelNode RewriterView::nodeAtTextCursorPosition(int cursorPosition) const
{
    return nodeAtTextCursorPositionHelper(rootModelNode(), cursorPosition);
}

bool RewriterView::renameId(const QString &oldId, const QString &newId)
{
    if (textModifier()) {
        PropertyName propertyName = oldId.toUtf8();

        bool hasAliasExport = rootModelNode().isValid()
                              && rootModelNode().hasBindingProperty(propertyName)
                              && rootModelNode().bindingProperty(propertyName).isAliasExport();

        auto instant = m_instantQmlTextUpdate;
        m_instantQmlTextUpdate = InstantQmlTextUpdate::Yes;

        bool refactoring = textModifier()->renameId(oldId, newId);

        m_instantQmlTextUpdate = instant;

        if (refactoring && hasAliasExport) { //Keep export alias properties
            rootModelNode().removeProperty(propertyName);
            PropertyName newPropertyName = newId.toUtf8();
            rootModelNode()
                .bindingProperty(newPropertyName)
                .setDynamicTypeNameAndExpression("alias", QString::fromUtf8(newPropertyName));
        }
        return refactoring;
    }

    return false;
}

#ifndef QDS_USE_PROJECTSTORAGE
const QmlJS::ScopeChain *RewriterView::scopeChain() const
{
    return textToModelMerger()->scopeChain();
}
#endif

const QmlJS::Document *RewriterView::document() const
{
    return textToModelMerger()->document();
}

inline static QString getUrlFromType(const QString &typeName)
{
    QStringList nameComponents = typeName.split('.');
    QString result;

    for (int i = 0; i < (nameComponents.size() - 1); i++) {
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
        simplifiedType = nameComponents.constLast();
    }

    QString alias;
    if (!url.isEmpty()) {
        for (const Import &import : model()->imports()) {
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

QStringList RewriterView::importDirectories() const
{
    return Utils::transform<QStringList>(m_textToModelMerger->vContext().paths,
                                         &Utils::FilePath::path);
}

QSet<QPair<QString, QString>> RewriterView::qrcMapping() const
{
    return m_textToModelMerger->qrcMapping();
}

namespace {
#ifdef QDS_USE_PROJECTSTORAGE

ModuleIds generateModuleIds(const ModelNodes &nodes)
{
    ModuleIds moduleIds;
    moduleIds.reserve(Utils::usize(nodes));
    for (const auto &node : nodes) {
        auto exportedNames = node.metaInfo().allExportedTypeNames();
        if (exportedNames.size())
            moduleIds.push_back(exportedNames.front().moduleId);
    }

    std::sort(moduleIds.begin(), moduleIds.end());
    moduleIds.erase(std::unique(moduleIds.begin(), moduleIds.end()), moduleIds.end());

    return moduleIds;
}

QStringList generateImports(ModuleIds moduleIds, const ProjectStorageType &projectStorage)
{
    QStringList imports;
    imports.reserve(std::ssize(moduleIds));

    for (auto moduleId : moduleIds) {
        using Storage::ModuleKind;
        auto module = projectStorage.module(moduleId);
        switch (module.kind) {
        case ModuleKind::QmlLibrary:
            imports.push_back("import " + module.name.toQString());
            break;
        case ModuleKind::PathLibrary:
            imports.push_back("import \"" + module.name.toQString() + "\"");
            break;
        case ModuleKind::CppLibrary:
            break;
        }
    }

    return imports;
}

QStringList generateImports(const ModelNodes &nodes)
{
    if (nodes.empty())
        return {};

    auto moduleIds = generateModuleIds(nodes);

    return generateImports(moduleIds, *nodes.front().model()->projectStorage());
}

#endif
} // namespace

void RewriterView::moveToComponent(const ModelNode &modelNode)
{
    if (!modelNode.isValid())
        return;

    int offset = nodeOffset(modelNode);

    const QList<ModelNode> nodes = modelNode.allSubModelNodesAndThisNode();
#ifdef QDS_USE_PROJECTSTORAGE
    auto directPaths = generateImports(nodes);
#else
    QSet<QString> directPathsSet;

    // Always add QtQuick import
    QString quickImport = model()->qtQuickItemMetaInfo().requiredImportString();
    if (!quickImport.isEmpty())
        directPathsSet.insert(quickImport);

    for (const ModelNode &partialNode : nodes) {
        QString importStr = partialNode.metaInfo().requiredImportString();
        if (importStr.size())
            directPathsSet << importStr;
    }

    auto directPaths = directPathsSet.values();
#endif

    QString importData = Utils::sorted(directPaths).join(QChar::LineFeed);
    if (importData.size())
        importData.append(QString(2, QChar::LineFeed));

    textModifier()->moveToComponent(offset, importData);
}

QStringList RewriterView::autoComplete(const QString &text, int pos, bool explicitComplete)
{
    QTextDocument textDocument;
    textDocument.setPlainText(text);

    QStringList list = textModifier()->autoComplete(&textDocument, pos, explicitComplete);
    list.removeDuplicates();

    return list;
}

#ifndef QDS_USE_PROJECTSTORAGE
QList<QmlTypeData> RewriterView::getQMLTypes() const
{
    QList<QmlTypeData> qmlDataList;

    qmlDataList.append(m_textToModelMerger->getQMLSingletons());

    for (const QmlJS::ModelManagerInterface::CppData &cppData :
         QmlJS::ModelManagerInterface::instance()->cppData())
        for (const LanguageUtils::FakeMetaObject::ConstPtr &fakeMetaObject : cppData.exportedTypes) {
            for (const LanguageUtils::FakeMetaObject::Export &exportItem : fakeMetaObject->exports()) {
                QmlTypeData qmlData;
                qmlData.cppClassName = fakeMetaObject->className();
                qmlData.typeName = exportItem.type;
                qmlData.importUrl = exportItem.package;
                qmlData.versionString = exportItem.version.toString();
                qmlData.superClassName = fakeMetaObject->superclassName();
                qmlData.isSingleton = fakeMetaObject->isSingleton();

                if (qmlData.importUrl != "<cpp>") //ignore pure unregistered cpp types
                    qmlDataList.append(qmlData);
            }
        }

    return qmlDataList;
}
#endif

void RewriterView::setWidgetStatusCallback(std::function<void(bool)> setWidgetStatusCallback)
{
    m_setWidgetStatusCallback = setWidgetStatusCallback;
}

void RewriterView::qmlTextChanged()
{
    if (inErrorState())
        return;
    QTC_ASSERT(m_textModifier, return);

    const QString newQmlText = m_textModifier->text();

#if 0
    qDebug() << Q_FUNC_INFO;
    qDebug() << "old:" << lastCorrectQmlSource;
    qDebug() << "new:" << newQmlText;
#endif

    switch (m_differenceHandling) {
    case Validate: {
        ModelValidator differenceHandler(m_textToModelMerger.get());
        if (m_textToModelMerger->load(newQmlText, differenceHandler))
            m_lastCorrectQmlSource = newQmlText;
        break;
    }

    case Amend: {
        if (m_instantQmlTextUpdate == InstantQmlTextUpdate::Yes
            || externalDependencies().instantQmlTextUpdate()) {
            amendQmlText();
        } else {
            if (externalDependencies().viewManagerUsesRewriterView(this)) {
                externalDependencies().viewManagerDiableWidgets();
                m_amendTimer.start();
            }
        }
        break;
    }
    }
}

void RewriterView::delayedSetup()
{
    if (m_textToModelMerger)
        m_textToModelMerger->delayedSetup();
}

QString RewriterView::getRawAuxiliaryData() const
{
    QTC_ASSERT(m_textModifier, return {});

    const QString oldText = m_textModifier->text();

    int startIndex = oldText.indexOf(annotationsStart);
    int endIndex = oldText.indexOf(annotationsEnd);

    if (startIndex > 0 && endIndex > 0)
        return oldText.mid(startIndex, endIndex - startIndex + annotationsEnd.length());

    return {};
}

void RewriterView::writeAuxiliaryData()
{
    QTC_ASSERT(m_textModifier, return);

    const QString oldText = m_textModifier->text();

    const int startIndex = oldText.indexOf(annotationsStart);
    const int endIndex = oldText.indexOf(annotationsEnd);

    QString auxData = auxiliaryDataAsQML();

    const bool replace = startIndex > 0 && endIndex > 0;

    if (!auxData.isEmpty()) {
        auxData.prepend("\n");
        auxData.prepend(annotationsStart);
        if (!replace)
            auxData.prepend("\n");
        auxData.append(annotationsEnd);
        if (!replace)
            auxData.append("\n");
    }

    if (replace)
        m_textModifier->replace(startIndex, endIndex - startIndex + annotationsEnd.length(), auxData);
    else
        m_textModifier->replace(oldText.length(), 0, auxData);
}

static void checkNode(const QmlJS::SimpleReaderNode::Ptr &node, RewriterView *view);

static void checkChildNodes(const QmlJS::SimpleReaderNode::Ptr &node, RewriterView *view)
{
    if (!node)
        return;

    for (const auto &child : node->children())
        checkNode(child, view);
}

static QString fixUpIllegalChars(const QString &str)
{
    QString ret = str;
    ret.replace("__AT__", "@");
    return ret;
}

void checkNode(const QmlJS::SimpleReaderNode::Ptr &node, RewriterView *view)
{
    if (!node)
        return;

    if (!node->propertyNames().contains("i"))
        return;

    const int index = node->property("i").value.toInt();

    const ModelNode modelNode = view->getNodeForCanonicalIndex(index);

    if (!modelNode.isValid())
        return;

    auto properties = node->properties();

    for (auto i = properties.begin(); i != properties.end(); ++i) {
        if (i.key() != "i") {
            const PropertyName name = fixUpIllegalChars(i.key()).toUtf8();
            if (!modelNode.hasAuxiliaryData(AuxiliaryDataType::Document, name))
                modelNode.setAuxiliaryData(AuxiliaryDataType::Document, name, i.value().value);
        }
    }

    checkChildNodes(node, view);
}

void RewriterView::restoreAuxiliaryData()
{
    QTC_ASSERT(m_textModifier, return);

    const char auxRestoredFlag[] = "AuxRestored@Internal";
    if (rootModelNode().hasAuxiliaryData(AuxiliaryDataType::Document, auxRestoredFlag))
        return;

    m_restoringAuxData = true;

    setupCanonicalHashes();

    if (m_canonicalIntModelNode.isEmpty())
        return;

    const QString text = m_textModifier->text();

    int startIndex = text.indexOf(annotationsStart);
    int endIndex = text.indexOf(annotationsEnd);

    if (startIndex > 0 && endIndex > 0) {
        const QString auxSource = text.mid(startIndex + annotationsStart.length(),
                                           endIndex - startIndex - annotationsStart.length());
        QmlJS::SimpleReader reader;
        checkChildNodes(reader.readFromSource(auxSource), this);
    }

    rootModelNode().setAuxiliaryData(AuxiliaryDataType::Document, auxRestoredFlag, true);
    m_restoringAuxData = false;
}

} // namespace QmlDesigner
