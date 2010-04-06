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
#include "nodeproperty.h"
#include "propertyparser.h"
#include "textmodifier.h"
#include "texttomodelmerger.h"
#include "rewriterview.h"
#include "variantproperty.h"

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/parser/qmljsast_p.h>

#include <QtDeclarative/QDeclarativeComponent>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtCore/QSet>

using namespace QmlJS;
using namespace QmlJS::AST;

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

    ExpressionStatement *exprStmt = cast<ExpressionStatement *>(script->statement);
    if (exprStmt)
        return isLiteralValue(exprStmt->expression);
    else
        return false;
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
    else if (typeName == QLatin1String("double"))
        return QMetaType::type("double");
    else if (typeName == QLatin1String("string"))
        return QMetaType::type("QString");
    else if (typeName == QLatin1String("url"))
        return QMetaType::type("QUrl");
    else if (typeName == QLatin1String("var"))
        return QMetaType::type("QVariant");
    else
        return -1;
}

static inline QVariant convertDynamicPropertyValueToVariant(const QString &astValue,
                                                            const QString &astType)
{
    const QString cleanedValue = stripQuotes(astValue.trimmed());

    if (astType.isEmpty())
        return QString();

    const int type = propertyType(astType);
    if (type == QMetaType::type("QVariant")) {
        if (cleanedValue.isNull()) // Explicitly isNull, NOT isEmpty!
            return QVariant(static_cast<QVariant::Type>(type));
        else
            return QVariant(cleanedValue);
    } else {
        QVariant value = QVariant(cleanedValue);
        value.convert(static_cast<QVariant::Type>(type));
        return value;
    }
}

} // anonymous namespace

namespace QmlDesigner {
namespace Internal {

class ReadingContext
{
public:
    ReadingContext(const Snapshot &snapshot, const Document::Ptr &doc,
                   const QStringList importPaths)
        : m_snapshot(snapshot)
        , m_doc(doc)
        , m_engine(new Interpreter::Engine)
        , m_context(new Interpreter::Context(m_engine))
        , m_link(m_context, doc, snapshot, importPaths)
        , m_scopeBuilder(doc, m_context)
    {
        m_context->build(QList<Node *>(), doc, snapshot, importPaths);
        m_link.scopeChainAt(doc);
    }

    ~ReadingContext()
    { delete m_context; delete m_engine; }

    Document::Ptr doc() const
    { return m_doc; }

    void enterScope(Node *node)
    { m_scopeBuilder.push(node); }

    void leaveScope()
    { m_scopeBuilder.pop(); }

    void lookup(UiQualifiedId *astTypeNode, QString &typeName, int &majorVersion,
                int &minorVersion, QString &defaultPropertyName)
    {
        const Interpreter::ObjectValue *value = m_context->lookupType(m_doc.data(), astTypeNode);
        const Interpreter::QmlObjectValue * qmlValue = dynamic_cast<const Interpreter::QmlObjectValue *>(value);
        if (qmlValue) {
            typeName = qmlValue->packageName() + QLatin1String("/") + qmlValue->className();
            majorVersion = qmlValue->majorVersion();
            minorVersion = qmlValue->minorVersion();
            defaultPropertyName = qmlValue->defaultPropertyName();
        } else if (value) {
            for (UiQualifiedId *iter = astTypeNode; iter; iter = iter->next)
                if (!iter->next && iter->name)
                    typeName = iter->name->asString();
            majorVersion = Interpreter::QmlObjectValue::NoVersion;
            minorVersion = Interpreter::QmlObjectValue::NoVersion;
        }
    }

    /// When something is changed here, also change Check::checkScopeObjectMember in
    /// qmljscheck.cpp
    /// ### Maybe put this into the context as a helper method.
    bool lookupProperty(const UiQualifiedId *id, const Interpreter::Value **property = 0, const Interpreter::ObjectValue **parentObject = 0, QString *name = 0)
    {
        QList<const Interpreter::ObjectValue *> scopeObjects = m_context->scopeChain().qmlScopeObjects;
        if (scopeObjects.isEmpty())
            return false;

        if (! id)
            return false; // ### error?

        if (! id->name) // possible after error recovery
            return false;

        QString propertyName = id->name->asString();
        if (name)
            *name = propertyName;

        if (propertyName == QLatin1String("id") && ! id->next)
            return false; // ### should probably be a special value

        // attached properties
        bool isAttachedProperty = false;
        if (! propertyName.isEmpty() && propertyName[0].isUpper()) {
            isAttachedProperty = true;
            scopeObjects += m_context->scopeChain().qmlTypes;
        }

        if (scopeObjects.isEmpty())
            return false;

        // global lookup for first part of id
        const Interpreter::ObjectValue *objectValue = 0;
        const Interpreter::Value *value = 0;
        for (int i = scopeObjects.size() - 1; i >= 0; --i) {
            objectValue = scopeObjects[i];
            value = objectValue->lookupMember(propertyName, m_context);
            if (value)
                break;
        }
        if (parentObject)
            *parentObject = objectValue;
        if (!value) {
            qWarning() << "Skipping invalid property name" << propertyName;
            return false;
        }

        // can't look up members for attached properties
        if (isAttachedProperty)
            return false;

        // member lookup
        const UiQualifiedId *idPart = id;
        while (idPart->next) {
            objectValue = Interpreter::value_cast<const Interpreter::ObjectValue *>(value);
            if (! objectValue) {
//                if (idPart->name)
//                    qDebug() << idPart->name->asString() << "has no property named"
//                             << propertyName;
                return false;
            }
            if (parentObject)
                *parentObject = objectValue;

            if (! idPart->next->name) {
                // somebody typed "id." and error recovery still gave us a valid tree,
                // so just bail out here.
                return false;
            }

            idPart = idPart->next;
            propertyName = idPart->name->asString();
            if (name)
                *name = propertyName;

            value = objectValue->lookupMember(propertyName, m_context);
            if (! value) {
//                if (idPart->name)
//                    qDebug() << "In" << idPart->name->asString() << ":"
//                             << objectValue->className() << "has no property named"
//                             << propertyName;
                return false;
            }
        }

        if (property)
            *property = value;
        return true;
    }

    bool isArrayProperty(const Interpreter::Value *value)
    {
        if (!value)
            return false;
        const Interpreter::ObjectValue *objectValue = value->asObjectValue();
        if (!objectValue)
            return false;

        return objectValue->prototype(m_context) == m_engine->arrayPrototype();
    }

    QVariant convertToVariant(const QString &astValue, UiQualifiedId *propertyId)
    {
        const QString cleanedValue = stripQuotes(astValue.trimmed());
        const Interpreter::Value *property = 0;
        const Interpreter::ObjectValue *containingObject = 0;
        QString name;
        if (!lookupProperty(propertyId, &property, &containingObject, &name)) {
            qWarning() << "Unknown property" << flatten(propertyId)
                       << "on line" << propertyId->identifierToken.startLine
                       << "column" << propertyId->identifierToken.startColumn;
            return QVariant(cleanedValue);
        }

        for (const Interpreter::ObjectValue *iter = containingObject; iter; iter = iter->prototype(m_context)) {
            if (iter->lookupMember(name, m_context, false)) {
                containingObject = iter;
                break;
            }
        }

        if (const Interpreter::QmlObjectValue * qmlObject = dynamic_cast<const Interpreter::QmlObjectValue *>(containingObject)) {
            const QString typeName = qmlObject->propertyType(name);
            if (qmlObject->isEnum(typeName)) {
                return QVariant(cleanedValue);
            } else {
                int type = QMetaType::type(typeName.toUtf8().constData());
                QVariant result;
                if (type)
                    result = PropertyParser::read(type, cleanedValue);
                if (result.isValid())
                    return result;
            }
        }

        QVariant v(cleanedValue);
        if (property->asBooleanValue()) {
            v.convert(QVariant::Bool);
        } else if (property->asColorValue()) {
            v.convert(QVariant::Color);
        } else if (property->asNumberValue()) {
            v.convert(QVariant::Double);
        } else if (property->asStringValue()) {
            // nothing to do
        } else if (property->asEasingCurveNameValue()) {
            // nothing to do
        }
        return v;
    }

private:
    Snapshot m_snapshot;
    Document::Ptr m_doc;
    Interpreter::Engine *m_engine;
    Interpreter::Context *m_context;
    Link m_link;
    ScopeBuilder m_scopeBuilder;
};

} // namespace Internal
} // namespace QmlDesigner

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

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
            const Import newImport = Import::createFileImport(strippedFileName,
                                                              version, as);

            if (!existingImports.remove(newImport))
                differenceHandler.modelMissesImport(newImport);
        } else {
            const Import newImport =
                    Import::createLibraryImport(flatten(import->importUri), as, version);

            if (!existingImports.remove(newImport))
                differenceHandler.modelMissesImport(newImport);
        }
    }

    foreach (const Import &import, existingImports)
        differenceHandler.importAbsentInQMl(import);
}

bool TextToModelMerger::load(const QString &data, DifferenceHandler &differenceHandler)
{
//    qDebug() << "TextToModelMerger::load with data:" << data;

    const QUrl url = m_rewriterView->model()->fileUrl();
    const QStringList importPaths = m_rewriterView->textModifier()->importPaths();
    setActive(true);

    { // Have the QML engine check if the document is valid:
        QDeclarativeEngine engine;
        foreach (const QString &importPath, importPaths)
            engine.addImportPath(importPath);
        QDeclarativeComponent comp(&engine);
        comp.setData(data.toUtf8(), url);
        if (comp.status() == QDeclarativeComponent::Error) {
            QList<RewriterView::Error> errors;
            foreach (const QDeclarativeError &error, comp.errors())
                errors.append(RewriterView::Error(error));
            m_rewriterView->setErrors(errors);
            setActive(false);
            return false;
        } else if (comp.status() == QDeclarativeComponent::Loading) {
            // Probably loading remote components. Previous DOM behaviour was:
            QList<RewriterView::Error> errors;
            errors.append(RewriterView::Error());
            m_rewriterView->setErrors(errors);
            setActive(false);
            return false;
        }
    }

    try {
        Snapshot snapshot = m_rewriterView->textModifier()->getSnapshot();
        const QString fileName = url.toLocalFile();
        Document::Ptr doc = Document::create(fileName.isEmpty() ? QLatin1String("<internal>") : fileName);
        doc->setSource(data);
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

        setActive(false);
        return true;
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

    m_rewriterView->positionStorage()->setNodeOffset(modelNode, astObjectType->identifierToken.offset);

    QString typeName, defaultPropertyName;
    int majorVersion;
    int minorVersion;
    context->lookup(astObjectType, typeName, majorVersion, minorVersion, defaultPropertyName);

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

    context->enterScope(astNode);

    QSet<QString> modelPropertyNames = QSet<QString>::fromList(modelNode.propertyNames());
    QList<UiObjectMember *> defaultPropertyItems;

    for (UiObjectMemberList *iter = astInitializer->members; iter; iter = iter->next) {
        UiObjectMember *member = iter->member;
        if (!member)
            continue;

        if (UiArrayBinding *array = cast<UiArrayBinding *>(member)) {
            const QString astPropertyName = flatten(array->qualifiedId);
            if (typeName == QLatin1String("Qt/PropertyChanges") || context->lookupProperty(array->qualifiedId)) {
                AbstractProperty modelProperty = modelNode.property(astPropertyName);
                QList<UiObjectMember *> arrayMembers;
                for (UiArrayMemberList *iter = array->members; iter; iter = iter->next)
                    if (UiObjectMember *member = iter->member)
                        arrayMembers.append(member);

                syncArrayProperty(modelProperty, arrayMembers, context, differenceHandler);
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
                const Interpreter::Value *propertyType;
                if (context->lookupProperty(binding->qualifiedId, &propertyType) || typeName == QLatin1String("Qt/PropertyChanges")) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName);
                    if (context->isArrayProperty(propertyType)) {
                        syncArrayProperty(modelProperty, QList<QmlJS::AST::UiObjectMember*>() << member, context, differenceHandler);
                    } else {
                        syncNodeProperty(modelProperty, binding, context, differenceHandler);
                    }
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
                astValue = textAt(context->doc(),
                                  script->statement->firstSourceLocation(),
                                  script->statement->lastSourceLocation());
                astValue = astValue.trimmed();
                if (astValue.endsWith(QLatin1Char(';')))
                    astValue = astValue.left(astValue.length() - 1);
                astValue = astValue.trimmed();
            }

            if (astPropertyName == QLatin1String("id")) {
                syncNodeId(modelNode, astValue, differenceHandler);
            } else if (isSignalPropertyName(astPropertyName)) {
                // skip signals
            } else if (isLiteralValue(script)) {
                if (typeName == QLatin1String("Qt/PropertyChanges")) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName);
                    const QVariant variantValue(astValue);
                    syncVariantProperty(modelProperty, variantValue, QString(), differenceHandler);
                    modelPropertyNames.remove(astPropertyName);
                } else {
                    const QVariant variantValue = context->convertToVariant(astValue, script->qualifiedId);
                    if (variantValue.isValid()) {
                        AbstractProperty modelProperty = modelNode.property(astPropertyName);
                        syncVariantProperty(modelProperty, variantValue, QString(), differenceHandler);
                        modelPropertyNames.remove(astPropertyName);
                    } else {
                        qWarning() << "Skipping invalid variant property" << astPropertyName
                                   << "for node type" << modelNode.type();
                    }
                }
            } else {
                if (typeName == QLatin1String("Qt/PropertyChanges") || context->lookupProperty(script->qualifiedId)) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName);
                    syncExpressionProperty(modelProperty, astValue, differenceHandler);
                    modelPropertyNames.remove(astPropertyName);
                } else {
                    qWarning() << "Skipping invalid expression property" << astPropertyName
                               << "for node type" << modelNode.type();
                }
            }
        } else if (UiPublicMember *property = cast<UiPublicMember *>(member)) {
            if (property->type == UiPublicMember::Signal)
                continue; // QML designer doesn't support this yet.

            if (!property->name || !property->memberType)
                continue; // better safe than sorry.

            const QString astName = property->name->asString();
            QString astValue;
            if (property->expression)
                astValue = textAt(context->doc(),
                                  property->expression->firstSourceLocation(),
                                  property->expression->lastSourceLocation());
            const QString astType = property->memberType->asString();
            AbstractProperty modelProperty = modelNode.property(astName);
            if (!property->expression || isLiteralValue(property->expression)) {
                const QVariant variantValue = convertDynamicPropertyValueToVariant(astValue, astType);
                syncVariantProperty(modelProperty, variantValue, astType, differenceHandler);
            } else {
                syncExpressionProperty(modelProperty, astValue, differenceHandler);
            }
            modelPropertyNames.remove(astName);
        } else {
            qWarning() << "Found an unknown QML value.";
        }
    }

    if (!defaultPropertyItems.isEmpty()) {
        if (defaultPropertyName.isEmpty()) {
            if (modelNode.type() != QLatin1String("Qt/Component"))
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

    context->leaveScope();
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
    QString typeName, dummy;
    int majorVersion;
    int minorVersion;
    context->lookup(binding->qualifiedTypeNameId, typeName, majorVersion, minorVersion, dummy);

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
                                          const QList<UiObjectMember *> &arrayMembers,
                                          ReadingContext *context,
                                          DifferenceHandler &differenceHandler)
{
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
                                            const QVariant &astValue,
                                            const QString &astType,
                                            DifferenceHandler &differenceHandler)
{
    if (modelProperty.isVariantProperty()) {
        VariantProperty modelVariantProperty = modelProperty.toVariantProperty();

        if (!equals(modelVariantProperty.value(), astValue)
                || !astType.isEmpty() != modelVariantProperty.isDynamic()
                || astType != modelVariantProperty.dynamicTypeName()) {
            differenceHandler.variantValuesDiffer(modelVariantProperty,
                                                  astValue,
                                                  astType);
        }
    } else {
        differenceHandler.shouldBeVariantProperty(modelProperty,
                                                  astValue,
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
//    qDebug()<< "ModelAmender::variantValuesDiffer for property"<<modelProperty.name()
//            << "in node" << modelProperty.parentModelNode().id()
//            << ", old value:" << modelProperty.value()
//            << "new value:" << qmlVariantValue;
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

    QString typeName, dummy;
    int majorVersion;
    int minorVersion;
    context->lookup(astObjectType, typeName, majorVersion, minorVersion, dummy);

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
        ObjectLengthCalculator objectLengthCalculator;
        unsigned length;
        if (objectLengthCalculator(componentText, offset, length)) {
            result = componentText.mid(offset, length);
        } else {
            result = componentText;
        }
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
