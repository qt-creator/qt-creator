/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "abstractproperty.h"
#include "bindingproperty.h"
#include "filemanager/firstdefinitionfinder.h"
#include "filemanager/objectlengthcalculator.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "propertymetainfo.h"
#include "textmodifier.h"
#include "texttomodelmerger.h"
#include "rewriterview.h"
#include "variantproperty.h"
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/parser/qmljsast_p.h>

#include <QDeclarativeEngine>
#include <QSet>
#include <private/qdeclarativedom_p.h>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlDesigner {
namespace Internal {

struct ReadingContext {
    ReadingContext(const Snapshot &snapshot, const Document::Ptr &doc,
                   const QStringList importPaths)
        : snapshot(snapshot)
        , doc(doc)
        , engine(new Interpreter::Engine)
        , ctxt(new Interpreter::Context(engine))
    {
        ctxt->build(QList<Node *>(), doc, snapshot, importPaths);
    }

    ~ReadingContext()
    { delete ctxt; delete engine; }

    void lookup(UiQualifiedId *astTypeNode, QString &typeName, int &majorVersion, int &minorVersion)
    {
        const Interpreter::ObjectValue *value = ctxt->lookupType(doc.data(), astTypeNode);
        if (const Interpreter::QmlObjectValue * qmlValue = dynamic_cast<const Interpreter::QmlObjectValue *>(value)) {
            typeName = qmlValue->packageName() + QLatin1String("/") + qmlValue->className();
            majorVersion = qmlValue->majorVersion();
            minorVersion = qmlValue->minorVersion();
        } else if (value) {
            for (UiQualifiedId *iter = astTypeNode; iter; iter = iter->next)
                if (!iter->next && iter->name)
                    typeName = iter->name->asString();
            majorVersion = Interpreter::QmlObjectValue::NoVersion;
            minorVersion = Interpreter::QmlObjectValue::NoVersion;
        }
    }

    Snapshot snapshot;
    Document::Ptr doc;
    Interpreter::Engine *engine;
    Interpreter::Context *ctxt;
};

} // namespace Internal
} // namespace QmlDesigner

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

namespace {

static inline QString stripQuotes(const QString &str)
{
    if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
            || (str.startsWith(QLatin1Char('\'')) && str.endsWith(QLatin1Char('\''))))
        return str.mid(1, str.length() - 2);

    return str;
}

static inline bool isSignalPropertyName(const QString &signalName)
{
    // see QmlCompiler::isSignalPropertyName
    return signalName.length() >= 3 && signalName.startsWith(QLatin1String("on")) &&
           signalName.at(2).isLetter();
}

static QString flatten(UiQualifiedId *qualifiedId)
{
    QString result;

    for (UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (!iter->name)
            continue;

        if (!result.isEmpty())
            result.append(QLatin1Char('.'));

        result.append(iter->name->asString());
    }

    return result;
}

static bool isLiteralValue(ExpressionNode *expr)
{
    if (cast<NumericLiteral*>(expr))
        return true;
    else if (cast<StringLiteral*>(expr))
        return true;
    else if (UnaryPlusExpression *plusExpr = cast<UnaryPlusExpression*>(expr))
        return isLiteralValue(plusExpr->expression);
    else if (UnaryMinusExpression *minusExpr = cast<UnaryMinusExpression*>(expr))
        return isLiteralValue(minusExpr->expression);
    else if (cast<TrueLiteral*>(expr))
        return true;
    else if (cast<FalseLiteral*>(expr))
        return true;
    else
        return false;
}

static inline bool isLiteralValue(UiScriptBinding *script)
{
    if (!script || !script->statement)
        return false;

    return isLiteralValue((cast<ExpressionStatement *>(script->statement))->expression);
}

static inline bool isValidPropertyForNode(const ModelNode &modelNode,
                                          const QString &propertyName)
{
    return modelNode.metaInfo().hasProperty(propertyName, true)
            || modelNode.type() == QLatin1String("Qt/PropertyChanges");
}

static inline int propertyType(const QString &typeName)
{
    if (typeName == QLatin1String("bool"))
        return QMetaType::type("bool");
    else if (typeName == QLatin1String("color"))
        return QMetaType::type("QColor");
    else if (typeName == QLatin1String("date"))
        return QMetaType::type("QDate");
    else if (typeName == QLatin1String("int"))
        return QMetaType::type("int");
    else if (typeName == QLatin1String("real"))
        return QMetaType::type("double");
    else if (typeName == QLatin1String("string"))
        return QMetaType::type("QString");
    else if (typeName == QLatin1String("url"))
        return QMetaType::type("QUrl");
    else if (typeName == QLatin1String("var") || typeName == QLatin1String("variant"))
        return QMetaType::type("QVariant");
    else
        return -1;
}

} // anonymous namespace

static inline bool equals(const QVariant &a, const QVariant &b)
{
    if (a.type() == QVariant::Double && b.type() == QVariant::Double)
        return qFuzzyCompare(a.toDouble(), b.toDouble());
    else
        return a == b;
}

TextToModelMerger::TextToModelMerger(RewriterView *reWriterView):
        m_rewriterView(reWriterView),
        m_isActive(false)
{
    Q_ASSERT(reWriterView);
}

void TextToModelMerger::setActive(bool active)
{
    m_isActive = active;
}

bool TextToModelMerger::isActive() const
{
    return m_isActive;
}

void TextToModelMerger::setupImports(const Document::Ptr &doc,
                                     DifferenceHandler &differenceHandler)
{
    QSet<Import> existingImports = m_rewriterView->model()->imports();

    for (UiImportList *iter = doc->qmlProgram()->imports; iter; iter = iter->next) {
        UiImport *import = iter->import;
        if (!import)
            continue;

        QString version;
        if (import->versionToken.isValid())
            version = textAt(doc, import->versionToken);
        QString as;
        if (import->importId)
            as = import->importId->asString();

        if (import->fileName) {
            const QString strippedFileName = stripQuotes(import->fileName->asString());
            Import newImport(Import::createFileImport(strippedFileName,
                                                      version, as));

            if (!existingImports.remove(newImport))
                differenceHandler.modelMissesImport(newImport);
        } else {
            Import newImport(Import::createLibraryImport(flatten(import->importUri),
                                                         as, version));

            if (!existingImports.remove(newImport))
                differenceHandler.modelMissesImport(newImport);
        }
    }

    foreach (const Import &import, existingImports)
        differenceHandler.importAbsentInQMl(import);
}

bool TextToModelMerger::load(const QByteArray &data, DifferenceHandler &differenceHandler)
{
    setActive(true);

    try {
        QDeclarativeEngine engine;
        QDeclarativeDomDocument domDoc;
        const QUrl url = m_rewriterView->model()->fileUrl();
        const bool success = domDoc.load(&engine, data, url);

        if (success) {
            Snapshot snapshot = m_rewriterView->textModifier()->getSnapshot();
            const QStringList importPaths = m_rewriterView->textModifier()->importPaths();
            const QString fileName = url.toLocalFile();
            Document::Ptr doc = Document::create(fileName);
            doc->setSource(QString::fromUtf8(data.constData()));
            doc->parseQml();
            snapshot.insert(doc);
            ReadingContext ctxt(snapshot, doc, importPaths);

            setupImports(doc, differenceHandler);

            UiObjectMember *astRootNode = 0;
            if (UiProgram *program = doc->qmlProgram())
                if (program->members)
                    astRootNode = program->members->member;
            ModelNode modelRootNode = m_rewriterView->rootModelNode();
            syncNode(modelRootNode, astRootNode, &ctxt, differenceHandler);
            m_rewriterView->positionStorage()->cleanupInvalidOffsets();
            m_rewriterView->clearErrors();
        } else {
            QList<RewriterView::Error> errors;
            foreach (const QDeclarativeError &qmlError, domDoc.errors())
                errors.append(RewriterView::Error(qmlError));
            m_rewriterView->setErrors(errors);
        }

        setActive(false);
        return success;
    } catch (Exception &e) {
        RewriterView::Error error(&e);
        // Somehow, the error below gets eaten in upper levels, so printing the
        // exception info here for debugging purposes:
        qDebug() << "*** An exception occurred while reading the QML file:"
                 << error.toString();
        m_rewriterView->addError(error);

        setActive(false);

        return false;
    }
}

void TextToModelMerger::syncNode(ModelNode &modelNode,
                                 UiObjectMember *astNode,
                                 ReadingContext *context,
                                 DifferenceHandler &differenceHandler)
{
    UiQualifiedId *astObjectType = 0;
    UiObjectInitializer *astInitializer = 0;
    if (UiObjectDefinition *def = cast<UiObjectDefinition *>(astNode)) {
        astObjectType = def->qualifiedTypeNameId;
        astInitializer = def->initializer;
    } else if (UiObjectBinding *bin = cast<UiObjectBinding *>(astNode)) {
        astObjectType = bin->qualifiedTypeNameId;
        astInitializer = bin->initializer;
    }

    if (!astObjectType || !astInitializer)
        return;

    m_rewriterView->positionStorage()->setNodeOffset(modelNode, astNode->firstSourceLocation().offset);

    QString typeName;
    int majorVersion;
    int minorVersion;
    context->lookup(astObjectType, typeName, majorVersion, minorVersion);

    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << flatten(astObjectType);
        return;
    }

    if (modelNode.type() != typeName
            /*|| modelNode.majorVersion() != domObject.objectTypeMajorVersion()
            || modelNode.minorVersion() != domObject.objectTypeMinorVersion()*/) {
        const bool isRootNode = m_rewriterView->rootModelNode() == modelNode;
        differenceHandler.typeDiffers(isRootNode, modelNode, typeName,
                                      majorVersion, minorVersion,
                                      astNode, context);
        if (!isRootNode)
            return; // the difference handler will create a new node, so we're done.
    }

    QSet<QString> modelPropertyNames = QSet<QString>::fromList(modelNode.propertyNames());
    QList<UiObjectMember *> defaultPropertyItems;

    for (UiObjectMemberList *iter = astInitializer->members; iter; iter = iter->next) {
        UiObjectMember *member = iter->member;
        if (!member)
            continue;

        if (UiArrayBinding *array = cast<UiArrayBinding *>(member)) {
            const QString astPropertyName = flatten(array->qualifiedId);
            if (isValidPropertyForNode(modelNode, astPropertyName)) {
                AbstractProperty modelProperty = modelNode.property(astPropertyName);
                syncArrayProperty(modelProperty, array, context, differenceHandler);
                modelPropertyNames.remove(astPropertyName);
            } else {
                qWarning() << "Skipping invalid array property" << astPropertyName
                           << "for node type" << modelNode.type();
            }
        } else if (cast<UiObjectDefinition *>(member)) {
            defaultPropertyItems.append(member);
        } else if (UiObjectBinding *binding = cast<UiObjectBinding *>(member)) {
            const QString astPropertyName = flatten(binding->qualifiedId);
            if (binding->hasOnToken) {
                // skip value sources
            } else {
                if (isValidPropertyForNode(modelNode, astPropertyName)) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName);
                    syncNodeProperty(modelProperty, binding, context, differenceHandler);
                    modelPropertyNames.remove(astPropertyName);
                } else {
                    qWarning() << "Skipping invalid node property" << astPropertyName
                               << "for node type" << modelNode.type();
                }
            }
        } else if (UiScriptBinding *script = cast<UiScriptBinding *>(member)) {
            const QString astPropertyName = flatten(script->qualifiedId);
            QString astValue;
            if (script->statement) {
                astValue = textAt(context->doc,
                                  script->statement->firstSourceLocation(),
                                  script->statement->lastSourceLocation());
                astValue = astValue.trimmed();
                if (astValue.endsWith(QLatin1Char(';')))
                    astValue = astValue.left(astValue.length() - 1);
            }

            if (astPropertyName == QLatin1String("id")) {
                syncNodeId(modelNode, astValue, differenceHandler);
            } else if (isSignalPropertyName(astPropertyName)) {
                // skip signals
            } else if (isLiteralValue(script)) {
                if (isValidPropertyForNode(modelNode, astPropertyName)) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName);
                    syncVariantProperty(modelProperty, astPropertyName, astValue, QString(), differenceHandler);
                    modelPropertyNames.remove(astPropertyName);
                } else {
                    qWarning() << "Skipping invalid variant property" << astPropertyName
                               << "for node type" << modelNode.type();
                }
            } else {
                if (isValidPropertyForNode(modelNode, astPropertyName)) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName);
                    syncExpressionProperty(modelProperty, astValue, differenceHandler);
                    modelPropertyNames.remove(astPropertyName);
                } else {
                    qWarning() << "Skipping invalid expression property" << astPropertyName
                               << "for node type" << modelNode.type();
                }
            }
        } else if (UiPublicMember *property = cast<UiPublicMember *>(member)) {
            const QString astName = property->name->asString();
            QString astValue;
            if (property->expression)
                astValue = textAt(context->doc,
                                  property->expression->firstSourceLocation(),
                                  property->expression->lastSourceLocation());
            const QString astType = property->memberType->asString();
            AbstractProperty modelProperty = modelNode.property(astName);
            if (!property->expression || isLiteralValue(property->expression))
                syncVariantProperty(modelProperty, astName, astValue, astType, differenceHandler);
            else
                syncExpressionProperty(modelProperty, astValue, differenceHandler);
            modelPropertyNames.remove(astName);
        } else {
            qWarning() << "Found an unknown QML value.";
        }
    }

    if (!defaultPropertyItems.isEmpty()) {
        QString defaultPropertyName = modelNode.metaInfo().defaultProperty();
        if (defaultPropertyName.isEmpty()) {
            qWarning() << "No default property for node type" << modelNode.type() << ", ignoring child items.";
        } else {
            AbstractProperty modelProperty = modelNode.property(defaultPropertyName);
            if (modelProperty.isNodeListProperty()) {
                NodeListProperty nodeListProperty = modelProperty.toNodeListProperty();
                syncNodeListProperty(nodeListProperty, defaultPropertyItems, context,
                                     differenceHandler);
            } else {
                differenceHandler.shouldBeNodeListProperty(modelProperty,
                                                           defaultPropertyItems,
                                                           context);
            }
            modelPropertyNames.remove(defaultPropertyName);
        }
    }

    foreach (const QString &modelPropertyName, modelPropertyNames) {
        AbstractProperty modelProperty = modelNode.property(modelPropertyName);

        // property deleted.
        differenceHandler.propertyAbsentFromQml(modelProperty);
    }
}

void TextToModelMerger::syncNodeId(ModelNode &modelNode, const QString &astObjectId,
                                   DifferenceHandler &differenceHandler)
{
    if (astObjectId.isEmpty()) {
        if (!modelNode.id().isEmpty()) {
            ModelNode existingNodeWithId = m_rewriterView->modelNodeForId(astObjectId);
            if (existingNodeWithId.isValid())
                existingNodeWithId.setId(QString());
            differenceHandler.idsDiffer(modelNode, astObjectId);
        }
    } else {
        if (modelNode.id() != astObjectId) {
            ModelNode existingNodeWithId = m_rewriterView->modelNodeForId(astObjectId);
            if (existingNodeWithId.isValid())
                existingNodeWithId.setId(QString());
            differenceHandler.idsDiffer(modelNode, astObjectId);
        }
    }
}

void TextToModelMerger::syncNodeProperty(AbstractProperty &modelProperty,
                                         UiObjectBinding *binding,
                                         ReadingContext *context,
                                         DifferenceHandler &differenceHandler)
{
    QString typeName;
    int majorVersion;
    int minorVersion;
    context->lookup(binding->qualifiedTypeNameId, typeName, majorVersion, minorVersion);

    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << flatten(binding->qualifiedTypeNameId);
        return;
    }

    if (modelProperty.isNodeProperty()) {
        ModelNode nodePropertyNode = modelProperty.toNodeProperty().modelNode();
        syncNode(nodePropertyNode, binding, context, differenceHandler);
    } else {
        differenceHandler.shouldBeNodeProperty(modelProperty,
                                               typeName,
                                               majorVersion,
                                               minorVersion,
                                               binding, context);
    }
}

void TextToModelMerger::syncExpressionProperty(AbstractProperty &modelProperty,
                                               const QString &javascript,
                                               DifferenceHandler &differenceHandler)
{
    if (modelProperty.isBindingProperty()) {
        BindingProperty bindingProperty = modelProperty.toBindingProperty();
        if (bindingProperty.expression() != javascript) {
            differenceHandler.bindingExpressionsDiffer(bindingProperty, javascript);
        }
    } else {
        differenceHandler.shouldBeBindingProperty(modelProperty, javascript);
    }
}

void TextToModelMerger::syncArrayProperty(AbstractProperty &modelProperty,
                                          UiArrayBinding *array,
                                          ReadingContext *context,
                                          DifferenceHandler &differenceHandler)
{
    QList<UiObjectMember *> arrayMembers;
    for (UiArrayMemberList *iter = array->members; iter; iter = iter->next)
        if (UiObjectMember *member = iter->member)
            arrayMembers.append(member);

    if (modelProperty.isNodeListProperty()) {
        NodeListProperty nodeListProperty = modelProperty.toNodeListProperty();
        syncNodeListProperty(nodeListProperty, arrayMembers, context, differenceHandler);
    } else {
        differenceHandler.shouldBeNodeListProperty(modelProperty,
                                                   arrayMembers,
                                                   context);
    }
}

void TextToModelMerger::syncVariantProperty(AbstractProperty &modelProperty,
                                            const QString &astName,
                                            const QString &astValue,
                                            const QString &astType,
                                            DifferenceHandler &differenceHandler)
{
    const QVariant astVariantValue = convertToVariant(modelProperty.parentModelNode(),
                                                      astName,
                                                      astValue,
                                                      astType);

    if (modelProperty.isVariantProperty()) {
        VariantProperty modelVariantProperty = modelProperty.toVariantProperty();

        if (!equals(modelVariantProperty.value(), astVariantValue)
                || !astType.isEmpty() != modelVariantProperty.isDynamic()
                || astType != modelVariantProperty.dynamicTypeName()) {
            differenceHandler.variantValuesDiffer(modelVariantProperty,
                                                  astVariantValue,
                                                  astType);
        }
    } else {
        differenceHandler.shouldBeVariantProperty(modelProperty,
                                                  astVariantValue,
                                                  astType);
    }
}

void TextToModelMerger::syncNodeListProperty(NodeListProperty &modelListProperty,
                                             const QList<UiObjectMember *> arrayMembers,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    QList<ModelNode> modelNodes = modelListProperty.toModelNodeList();
    int i = 0;
    for (; i < modelNodes.size() && i < arrayMembers.size(); ++i) {
        ModelNode modelNode = modelNodes.at(i);
        syncNode(modelNode, arrayMembers.at(i), context, differenceHandler);
    }

    for (int j = i; j < arrayMembers.size(); ++j) {
        // more elements in the dom-list, so add them to the model
        UiObjectMember *arrayMember = arrayMembers.at(j);
        const ModelNode newNode = differenceHandler.listPropertyMissingModelNode(modelListProperty, context, arrayMember);
        QString name;
        if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(arrayMember))
            name = flatten(definition->qualifiedTypeNameId);
        // TODO: resolve name here!
        if (name == QLatin1String("Qt/Component"))
            setupComponent(newNode);
    }

    for (int j = i; j < modelNodes.size(); ++j) {
        // more elements in the model, so remove them.
        ModelNode modelNode = modelNodes.at(j);
        differenceHandler.modelNodeAbsentFromQml(modelNode);
    }
}

ModelNode TextToModelMerger::createModelNode(const QString &typeName,
                                             int majorVersion,
                                             int minorVersion,
                                             UiObjectMember *astNode,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    ModelNode newNode = m_rewriterView->createModelNode(typeName,
                                                        majorVersion,
                                                        minorVersion);
    syncNode(newNode, astNode, context, differenceHandler);
    return newNode;
}

QVariant TextToModelMerger::convertToVariant(const ModelNode &node,
                                             const QString &astName,
                                             const QString &astValue,
                                             const QString &astType)
{
    const QString cleanedValue = stripQuotes(astValue.trimmed());

    if (!astType.isEmpty()) {
        const int type = propertyType(astType);
        QVariant value(cleanedValue);
        value.convert(static_cast<QVariant::Type>(type));
        return value;
    }

    const NodeMetaInfo nodeMetaInfo = node.metaInfo();

    if (nodeMetaInfo.isValid()) {
        const PropertyMetaInfo propertyMetaInfo = nodeMetaInfo.property(astName, true);

        if (propertyMetaInfo.isValid()) {
            QVariant castedValue = propertyMetaInfo.castedValue(cleanedValue);
            if (!castedValue.isValid())
                qWarning() << "Casting the value" << cleanedValue
                           << "of property" << astName
                           << "to the property type" << propertyMetaInfo.type()
                           << "failed";
            return castedValue;
        } else if (node.type() == QLatin1String("Qt/PropertyChanges")) {
            // In the future, we should do the type resolving in a second pass, or delay setting properties until the full file has been parsed.
            return QVariant(cleanedValue);
        } else {
            qWarning() << "Unknown property" << astName
                       << "in node" << node.type()
                       << "with value" << cleanedValue;
            return QVariant();
        }
    } else {
        qWarning() << "Unknown property" << astName
                   << "in node" << node.type()
                   << "with value" << cleanedValue;
        return QVariant::fromValue(cleanedValue);
    }
}

void ModelValidator::modelMissesImport(const Import &import)
{
    Q_ASSERT(m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::importAbsentInQMl(const Import &import)
{
    Q_ASSERT(! m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::bindingExpressionsDiffer(BindingProperty &modelProperty,
                                              const QString &javascript)
{
    Q_ASSERT(modelProperty.expression() == javascript);
    Q_ASSERT(0);
}

void ModelValidator::shouldBeBindingProperty(AbstractProperty &modelProperty,
                                             const QString &/*javascript*/)
{
    Q_ASSERT(modelProperty.isBindingProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                              const QList<UiObjectMember *> /*arrayMembers*/,
                                              ReadingContext * /*context*/)
{
    Q_ASSERT(modelProperty.isNodeListProperty());
    Q_ASSERT(0);
}

void ModelValidator::variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName)
{
    Q_ASSERT(modelProperty.isDynamic() == !dynamicTypeName.isEmpty());
    if (modelProperty.isDynamic()) {
        Q_ASSERT(modelProperty.dynamicTypeName() == dynamicTypeName);
    }

    Q_ASSERT(equals(modelProperty.value(), qmlVariantValue));
    Q_ASSERT(0);
}

void ModelValidator::shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &/*qmlVariantValue*/, const QString &/*dynamicTypeName*/)
{
    Q_ASSERT(modelProperty.isVariantProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeProperty(AbstractProperty &modelProperty,
                                          const QString &/*typeName*/,
                                          int /*majorVersion*/,
                                          int /*minorVersion*/,
                                          UiObjectMember * /*astNode*/,
                                          ReadingContext * /*context*/)
{
    Q_ASSERT(modelProperty.isNodeProperty());
    Q_ASSERT(0);
}

void ModelValidator::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    Q_ASSERT(!modelNode.isValid());
    Q_ASSERT(0);
}

ModelNode ModelValidator::listPropertyMissingModelNode(NodeListProperty &/*modelProperty*/,
                                                       ReadingContext * /*context*/,
                                                       UiObjectMember * /*arrayMember*/)
{
    Q_ASSERT(0);
    return ModelNode();
}

void ModelValidator::typeDiffers(bool /*isRootNode*/,
                                 ModelNode &modelNode,
                                 const QString &typeName,
                                 int majorVersion,
                                 int minorVersion,
                                 QmlJS::AST::UiObjectMember * /*astNode*/,
                                 ReadingContext * /*context*/)
{
    Q_ASSERT(modelNode.type() == typeName);
    Q_ASSERT(modelNode.majorVersion() == majorVersion);
    Q_ASSERT(modelNode.minorVersion() == minorVersion);
    Q_ASSERT(0);
}

void ModelValidator::propertyAbsentFromQml(AbstractProperty &modelProperty)
{
    Q_ASSERT(!modelProperty.isValid());
    Q_ASSERT(0);
}

void ModelValidator::idsDiffer(ModelNode &modelNode, const QString &qmlId)
{
    Q_ASSERT(modelNode.id() == qmlId);
    Q_ASSERT(0);
}

void ModelAmender::modelMissesImport(const Import &import)
{
    m_merger->view()->model()->addImport(import);
}

void ModelAmender::importAbsentInQMl(const Import &import)
{
    m_merger->view()->model()->removeImport(import);
}

void ModelAmender::bindingExpressionsDiffer(BindingProperty &modelProperty,
                                            const QString &javascript)
{
    modelProperty.toBindingProperty().setExpression(javascript);
}

void ModelAmender::shouldBeBindingProperty(AbstractProperty &modelProperty,
                                           const QString &javascript)
{
    ModelNode theNode = modelProperty.parentModelNode();
    BindingProperty newModelProperty = theNode.bindingProperty(modelProperty.name());
    newModelProperty.setExpression(javascript);
}

void ModelAmender::shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                            const QList<UiObjectMember *> arrayMembers,
                                            ReadingContext *context)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeListProperty newNodeListProperty = theNode.nodeListProperty(modelProperty.name());
    m_merger->syncNodeListProperty(newNodeListProperty,
                                   arrayMembers,
                                   context,
                                   *this);
}

void ModelAmender::variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicType)
{
    if (dynamicType.isEmpty())
        modelProperty.setValue(qmlVariantValue);
    else
        modelProperty.setDynamicTypeNameAndValue(dynamicType, qmlVariantValue);
}

void ModelAmender::shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName)
{
    ModelNode theNode = modelProperty.parentModelNode();
    VariantProperty newModelProperty = theNode.variantProperty(modelProperty.name());

    if (dynamicTypeName.isEmpty())
        newModelProperty.setValue(qmlVariantValue);
    else
        newModelProperty.setDynamicTypeNameAndValue(dynamicTypeName, qmlVariantValue);
}

void ModelAmender::shouldBeNodeProperty(AbstractProperty &modelProperty,
                                        const QString &typeName,
                                        int majorVersion,
                                        int minorVersion,
                                        UiObjectMember *astNode,
                                        ReadingContext *context)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeProperty newNodeProperty = theNode.nodeProperty(modelProperty.name());
    newNodeProperty.setModelNode(m_merger->createModelNode(typeName,
                                                           majorVersion,
                                                           minorVersion,
                                                           astNode,
                                                           context,
                                                           *this));
}

void ModelAmender::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    modelNode.destroy();
}

ModelNode ModelAmender::listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                                     ReadingContext *context,
                                                     UiObjectMember *arrayMember)
{
    UiQualifiedId *astObjectType = 0;
    UiObjectInitializer *astInitializer = 0;
    if (UiObjectDefinition *def = cast<UiObjectDefinition *>(arrayMember)) {
        astObjectType = def->qualifiedTypeNameId;
        astInitializer = def->initializer;
    } else if (UiObjectBinding *bin = cast<UiObjectBinding *>(arrayMember)) {
        astObjectType = bin->qualifiedTypeNameId;
        astInitializer = bin->initializer;
    }

    if (!astObjectType || !astInitializer)
        return ModelNode();

    QString typeName;
    int majorVersion;
    int minorVersion;
    context->lookup(astObjectType, typeName, majorVersion, minorVersion);

    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << flatten(astObjectType);
        return ModelNode();
    }

    const ModelNode &newNode = m_merger->createModelNode(typeName,
                                                         majorVersion,
                                                         minorVersion,
                                                         arrayMember,
                                                         context,
                                                         *this);
    modelProperty.reparentHere(newNode);
    return newNode;
}

void ModelAmender::typeDiffers(bool isRootNode,
                               ModelNode &modelNode,
                               const QString &typeName,
                               int majorVersion,
                               int minorVersion,
                               QmlJS::AST::UiObjectMember *astNode,
                               ReadingContext *context)
{
    if (isRootNode) {
        modelNode.view()->changeRootNodeType(typeName, majorVersion, minorVersion);
    } else {
        NodeAbstractProperty parentProperty = modelNode.parentProperty();
        int nodeIndex = -1;
        if (parentProperty.isNodeListProperty()) {
            nodeIndex = parentProperty.toNodeListProperty().toModelNodeList().indexOf(modelNode);
            Q_ASSERT(nodeIndex >= 0);
        }

        modelNode.destroy();

        const ModelNode &newNode = m_merger->createModelNode(typeName,
                                                             majorVersion,
                                                             minorVersion,
                                                             astNode,
                                                             context,
                                                             *this);
        parentProperty.reparentHere(newNode);
        if (nodeIndex >= 0) {
            int currentIndex = parentProperty.toNodeListProperty().toModelNodeList().indexOf(newNode);
            if (nodeIndex != currentIndex)
                parentProperty.toNodeListProperty().slide(currentIndex, nodeIndex);
        }
    }
}

void ModelAmender::propertyAbsentFromQml(AbstractProperty &modelProperty)
{
    modelProperty.parentModelNode().removeProperty(modelProperty.name());
}

void ModelAmender::idsDiffer(ModelNode &modelNode, const QString &qmlId)
{
    modelNode.setId(qmlId);
}

void TextToModelMerger::setupComponent(const ModelNode &node)
{
    Q_ASSERT(node.type() == QLatin1String("Qt/Component"));

    QString componentText = m_rewriterView->extractText(QList<ModelNode>() << node).value(node);

    if (componentText.isEmpty())
        return;

    QString result;
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

QString TextToModelMerger::textAt(const Document::Ptr &doc,
                                  const SourceLocation &location)
{
    return doc->source().mid(location.offset, location.length);
}

QString TextToModelMerger::textAt(const Document::Ptr &doc,
                                  const SourceLocation &from,
                                  const SourceLocation &to)
{
    return doc->source().mid(from.offset, to.end() - from.begin());
}
