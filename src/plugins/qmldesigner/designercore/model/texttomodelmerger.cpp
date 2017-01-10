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

#include "texttomodelmerger.h"

#include "documentmessage.h"
#include "modelnodepositionstorage.h"
#include "abstractproperty.h"
#include "bindingproperty.h"
#include "filemanager/firstdefinitionfinder.h"
#include "filemanager/objectlengthcalculator.h"
#include "filemanager/qmlrefactoring.h"
#include "nodeproperty.h"
#include "propertyparser.h"
#include "rewriterview.h"
#include "variantproperty.h"
#include "signalhandlerproperty.h"
#include "nodemetainfo.h"
#include "enumeration.h"

#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljslink.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsutils.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsqrcparser.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsvalueowner.h>

#include <utils/qtcassert.h>

#include <QSet>
#include <QDir>
#include <QLoggingCategory>

using namespace LanguageUtils;
using namespace QmlJS;

static Q_LOGGING_CATEGORY(rewriterBenchmark, "qtc.rewriter.load")

namespace {

static inline bool isSupportedAttachedProperties(const QString &propertyName)
{
    return propertyName.startsWith(QLatin1String("Layout."));
}

static inline QStringList supportedVersionsList()
{
    static const QStringList list = {
        "2.0", "2.1", "2.2", "2.3", "2.4", "2.5", "2.6", "2.7", "2.8"
    };
    return list;
}

static inline QStringList globalQtEnums()
{
    static const QStringList list = {
        "Horizontal", "Vertical", "AlignVCenter", "AlignLeft", "LeftToRight", "RightToLeft",
        "AlignHCenter", "AlignRight", "AlignBottom", "AlignBaseline", "AlignTop", "BottomLeft",
        "LeftEdge", "RightEdge", "BottomEdge"
    };

    return list;
}

static inline QStringList knownEnumScopes()
{
    static const QStringList list = {
        "TextInput", "TextEdit", "Material", "Universal", "Font"
    };
    return list;
}

static inline bool supportedQtQuickVersion(const QString &version)
{
    return supportedVersionsList().contains(version);
}

static inline QString stripQuotes(const QString &str)
{
    if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
            || (str.startsWith(QLatin1Char('\'')) && str.endsWith(QLatin1Char('\''))))
        return str.mid(1, str.length() - 2);

    return str;
}

static inline QString deEscape(const QString &value)
{
    QString result = value;

    result.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    result.replace(QStringLiteral("\\\""), QStringLiteral("\""));
    result.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
    result.replace(QStringLiteral("\\r"), QStringLiteral("\\\r"));
    result.replace(QStringLiteral("\\n"), QStringLiteral("\n"));

    return result;
}

static inline unsigned char convertHex(ushort c)
{
    if (c >= '0' && c <= '9')
        return (c - '0');
    else if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    else
        return (c - 'A' + 10);
}

QChar convertUnicode(ushort c1, ushort c2,
                             ushort c3, ushort c4)
{
    return QChar((convertHex(c3) << 4) + convertHex(c4),
                  (convertHex(c1) << 4) + convertHex(c2));
}

static inline bool isHexDigit(ushort c)
{
    return ((c >= '0' && c <= '9')
            || (c >= 'a' && c <= 'f')
            || (c >= 'A' && c <= 'F'));
}


static inline QString fixEscapedUnicodeChar(const QString &value) //convert "\u2939"
{
    if (value.count() == 6 && value.at(0) == QLatin1Char('\\') && value.at(1) == QLatin1Char('u') &&
        isHexDigit(value.at(2).unicode()) && isHexDigit(value.at(3).unicode()) &&
        isHexDigit(value.at(4).unicode()) && isHexDigit(value.at(5).unicode())) {
            return convertUnicode(value.at(2).unicode(), value.at(3).unicode(), value.at(4).unicode(), value.at(5).unicode());
    }
    return value;
}

static inline bool isSignalPropertyName(const QString &signalName)
{
    if (signalName.isEmpty())
        return false;
    // see QmlCompiler::isSignalPropertyName
    QStringList list = signalName.split(QLatin1String("."));

    QString pureSignalName = list.last();
    return pureSignalName.length() >= 3 && pureSignalName.startsWith(QStringLiteral("on")) &&
            pureSignalName.at(2).isLetter();
}

static inline QVariant cleverConvert(const QString &value)
{
    if (value == QLatin1String("true"))
        return QVariant(true);
    if (value == QLatin1String("false"))
        return QVariant(false);
    bool flag;
    int i = value.toInt(&flag);
    if (flag)
        return QVariant(i);
    double d = value.toDouble(&flag);
    if (flag)
        return QVariant(d);
    return QVariant(value);
}

static bool isLiteralValue(AST::ExpressionNode *expr)
{
    if (AST::cast<AST::NumericLiteral*>(expr))
        return true;
    else if (AST::cast<AST::StringLiteral*>(expr))
        return true;
    else if (AST::UnaryPlusExpression *plusExpr = AST::cast<AST::UnaryPlusExpression*>(expr))
        return isLiteralValue(plusExpr->expression);
    else if (AST::UnaryMinusExpression *minusExpr = AST::cast<AST::UnaryMinusExpression*>(expr))
        return isLiteralValue(minusExpr->expression);
    else if (AST::cast<AST::TrueLiteral*>(expr))
        return true;
    else if (AST::cast<AST::FalseLiteral*>(expr))
        return true;
    else
        return false;
}

static bool isLiteralValue(AST::Statement *stmt)
{
    AST::ExpressionStatement *exprStmt = AST::cast<AST::ExpressionStatement *>(stmt);
    if (exprStmt)
        return isLiteralValue(exprStmt->expression);
    else
        return false;
}

static inline bool isLiteralValue(AST::UiScriptBinding *script)
{
    if (!script || !script->statement)
        return false;

    return isLiteralValue(script->statement);
}

static inline int propertyType(const QString &typeName)
{
    if (typeName == QStringLiteral("bool"))
        return QMetaType::type("bool");
    else if (typeName == QStringLiteral("color"))
        return QMetaType::type("QColor");
    else if (typeName == QStringLiteral("date"))
        return QMetaType::type("QDate");
    else if (typeName == QStringLiteral("int"))
        return QMetaType::type("int");
    else if (typeName == QStringLiteral("real"))
        return QMetaType::type("double");
    else if (typeName == QStringLiteral("double"))
        return QMetaType::type("double");
    else if (typeName == QStringLiteral("string"))
        return QMetaType::type("QString");
    else if (typeName == QStringLiteral("url"))
        return QMetaType::type("QUrl");
    else if (typeName == QStringLiteral("var") || typeName == QStringLiteral("variant"))
        return QMetaType::type("QVariant");
    else
        return -1;
}

static inline QVariant convertDynamicPropertyValueToVariant(const QString &astValue,
                                                            const QString &astType)
{
    const QString cleanedValue = fixEscapedUnicodeChar(deEscape(stripQuotes(astValue.trimmed())));

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

static bool isListElementType(const QmlDesigner::TypeName &type)
{
    return type == "ListElement" || type == "QtQuick.ListElement" || type == "Qt.ListElement";
}

static bool isComponentType(const QmlDesigner::TypeName &type)
{
    return type == "Component"
            || type == "Qt.Component"
            || type == "QtQuick.Component"
            || type == "<cpp>.QQmlComponent"
            || type == "QQmlComponent";
}

static bool isCustomParserType(const QmlDesigner::TypeName &type)
{
    return type == "QtQuick.VisualItemModel" || type == "Qt.VisualItemModel" ||
           type == "QtQuick.VisualDataModel" || type == "Qt.VisualDataModel" ||
           type == "QtQuick.ListModel" || type == "Qt.ListModel" ||
           type == "QtQuick.XmlListModel" || type == "Qt.XmlListModel";
}


static bool isPropertyChangesType(const QmlDesigner::TypeName &type)
{
    return type == "PropertyChanges" || type == "QtQuick.PropertyChanges" || type == "Qt.PropertyChanges";
}

static bool isConnectionsType(const QmlDesigner::TypeName &type)
{
    return type == "Connections" || type == "QtQuick.Connections" || type == "Qt.Connections";
}

static bool propertyIsComponentType(const QmlDesigner::NodeAbstractProperty &property, const QmlDesigner::TypeName &type, QmlDesigner::Model *model)
{
    if (model->metaInfo(type).isSubclassOf("QtQuick.Component") && !isComponentType(type))
        return false; //If the type is already a subclass of Component keep it

    return property.parentModelNode().isValid() &&
            isComponentType(property.parentModelNode().metaInfo().propertyTypeName(property.name()));
}

static inline QString extractComponentFromQml(const QString &source)
{
    if (source.isEmpty())
        return QString();

    QString result;
    if (source.contains(QLatin1String("Component"))) { //explicit component
        QmlDesigner::FirstDefinitionFinder firstDefinitionFinder(source);
        int offset = firstDefinitionFinder(0);
        if (offset < 0)
            return QString(); //No object definition found
        QmlDesigner::ObjectLengthCalculator objectLengthCalculator;
        unsigned length;
        if (objectLengthCalculator(source, offset, length))
            result = source.mid(offset, length);
        else
            result = source;
    } else {
        result = source; //implicit component
    }
    return result;
}

} // anonymous namespace

namespace QmlDesigner {
namespace Internal {

class ReadingContext
{
public:
    ReadingContext(const Snapshot &snapshot, const Document::Ptr &doc,
                   const ViewerContext &vContext)
        : m_snapshot(snapshot)
        , m_doc(doc)
        , m_link(snapshot, vContext,
                 ModelManagerInterface::instance()->builtins(doc))
        , m_context(m_link(doc, &m_diagnosticLinkMessages))
        , m_scopeChain(doc, m_context)
        , m_scopeBuilder(&m_scopeChain)
    {
    }

    ~ReadingContext()
    {}

    Document::Ptr doc() const
    { return m_doc; }

    void enterScope(AST::Node *node)
    { m_scopeBuilder.push(node); }

    void leaveScope()
    { m_scopeBuilder.pop(); }

    void lookup(AST::UiQualifiedId *astTypeNode, QString &typeName, int &majorVersion,
                int &minorVersion, QString &defaultPropertyName)
    {
        const ObjectValue *value = m_context->lookupType(m_doc.data(), astTypeNode);
        defaultPropertyName = m_context->defaultPropertyName(value);

        const CppComponentValue *qmlValue = value_cast<CppComponentValue>(value);
        if (qmlValue) {
            typeName = qmlValue->moduleName() + QStringLiteral(".") + qmlValue->className();

            majorVersion = qmlValue->componentVersion().majorVersion();
            minorVersion = qmlValue->componentVersion().minorVersion();
        } else {
            for (AST::UiQualifiedId *iter = astTypeNode; iter; iter = iter->next)
                if (!iter->next && !iter->name.isEmpty())
                    typeName = iter->name.toString();

            QString fullTypeName;
            for (AST::UiQualifiedId *iter = astTypeNode; iter; iter = iter->next)
                if (!iter->name.isEmpty())
                    fullTypeName += iter->name.toString() + QLatin1Char('.');

            if (fullTypeName.endsWith(QLatin1Char('.')))
                fullTypeName.chop(1);

            majorVersion = ComponentVersion::NoVersion;
            minorVersion = ComponentVersion::NoVersion;

            const Imports *imports = m_context->imports(m_doc.data());
            ImportInfo importInfo = imports->info(fullTypeName, m_context.data());
            if (importInfo.isValid() && importInfo.type() == ImportType::Library) {
                QString name = importInfo.name();
                majorVersion = importInfo.version().majorVersion();
                minorVersion = importInfo.version().minorVersion();
                typeName.prepend(name + QLatin1Char('.'));
            } else if (importInfo.isValid() && importInfo.type() == ImportType::Directory) {
                QString path = importInfo.path();
                QDir dir(m_doc->path());
                // should probably try to make it relatve to some import path, not to the document path
                QString relativeDir = dir.relativeFilePath(path);
                QString name = relativeDir.replace(QLatin1Char('/'), QLatin1Char('.'));
                if (!name.isEmpty() && name != QLatin1String("."))
                    typeName.prepend(name + QLatin1Char('.'));
            } else if (importInfo.isValid() && importInfo.type() == ImportType::QrcDirectory) {
                QString path = QrcParser::normalizedQrcDirectoryPath(importInfo.path());
                path = path.mid(1, path.size() - ((path.size() > 1) ? 2 : 1));
                const QString name = path.replace(QLatin1Char('/'), QLatin1Char('.'));
                if (!name.isEmpty())
                    typeName.prepend(name + QLatin1Char('.'));
            }
        }
    }

    /// When something is changed here, also change Check::checkScopeObjectMember in
    /// qmljscheck.cpp
    /// ### Maybe put this into the context as a helper function.
    bool lookupProperty(const QString &prefix, const AST::UiQualifiedId *id, const Value **property = 0, const ObjectValue **parentObject = 0, QString *name = 0)
    {
        QList<const ObjectValue *> scopeObjects = m_scopeChain.qmlScopeObjects();
        if (scopeObjects.isEmpty())
            return false;

        if (!id)
            return false; // ### error?

        if (id->name.isEmpty()) // possible after error recovery
            return false;

        QString propertyName;
        if (prefix.isEmpty())
            propertyName = id->name.toString();
        else
            propertyName = prefix;

        if (name)
            *name = propertyName;

        if (propertyName == QStringLiteral("id") && ! id->next)
            return false; // ### should probably be a special value

        // attached properties
        bool isAttachedProperty = false;
        if (! propertyName.isEmpty() && propertyName[0].isUpper()) {
            isAttachedProperty = true;
            if (const ObjectValue *qmlTypes = m_scopeChain.qmlTypes())
                scopeObjects += qmlTypes;
        }

        if (scopeObjects.isEmpty())
            return false;

        // global lookup for first part of id
        const ObjectValue *objectValue = 0;
        const Value *value = 0;
        for (int i = scopeObjects.size() - 1; i >= 0; --i) {
            objectValue = scopeObjects[i];
            value = objectValue->lookupMember(propertyName, m_context);
            if (value)
                break;
        }
        if (parentObject)
            *parentObject = objectValue;
        if (!value) {
            qWarning() << Q_FUNC_INFO << "Skipping invalid property name" << propertyName;
            return false;
        }

        // can't look up members for attached properties
        if (isAttachedProperty)
            return false;

        // resolve references
        if (const Reference *ref = value->asReference())
            value = m_context->lookupReference(ref);

        // member lookup
        const AST::UiQualifiedId *idPart = id;
        if (prefix.isEmpty())
            idPart = idPart->next;
        for (; idPart; idPart = idPart->next) {
            objectValue = value_cast<ObjectValue>(value);
            if (! objectValue) {
//                if (idPart->name)
//                    qDebug() << idPart->name->asString() << "has no property named"
//                             << propertyName;
                return false;
            }
            if (parentObject)
                *parentObject = objectValue;

            if (idPart->name.isEmpty()) {
                // somebody typed "id." and error recovery still gave us a valid tree,
                // so just bail out here.
                return false;
            }

            propertyName = idPart->name.toString();
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

    bool isArrayProperty(const Value *value, const ObjectValue *containingObject, const QString &name)
    {
        if (!value)
            return false;
        const ObjectValue *objectValue = value->asObjectValue();
        if (objectValue && objectValue->prototype(m_context) == m_context->valueOwner()->arrayPrototype())
            return true;

        PrototypeIterator iter(containingObject, m_context);
        while (iter.hasNext()) {
            const ObjectValue *proto = iter.next();
            if (proto->lookupMember(name, m_context) == m_context->valueOwner()->arrayPrototype())
                return true;
            if (const CppComponentValue *qmlIter = value_cast<CppComponentValue>(proto)) {
                if (qmlIter->isListProperty(name))
                    return true;
            }
        }
        return false;
    }

    QVariant convertToVariant(const QString &astValue, const QString &propertyPrefix, AST::UiQualifiedId *propertyId)
    {
        const bool hasQuotes = astValue.trimmed().left(1) == QStringLiteral("\"") && astValue.trimmed().right(1) == QStringLiteral("\"");
        const QString cleanedValue = fixEscapedUnicodeChar(deEscape(stripQuotes(astValue.trimmed())));
        const Value *property = 0;
        const ObjectValue *containingObject = 0;
        QString name;
        if (!lookupProperty(propertyPrefix, propertyId, &property, &containingObject, &name)) {
            qWarning() << Q_FUNC_INFO << "Unknown property" << propertyPrefix + QLatin1Char('.') + toString(propertyId)
                       << "on line" << propertyId->identifierToken.startLine
                       << "column" << propertyId->identifierToken.startColumn;
            return hasQuotes ? QVariant(cleanedValue) : cleverConvert(cleanedValue);
        }

        if (containingObject)
            containingObject->lookupMember(name, m_context, &containingObject);

        if (const CppComponentValue * qmlObject = value_cast<CppComponentValue>(containingObject)) {
            const QString typeName = qmlObject->propertyType(name);
            if (qmlObject->getEnum(typeName).isValid()) {
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

        if (property->asColorValue())
            return PropertyParser::read(QVariant::Color, cleanedValue);
        else if (property->asUrlValue())
            return PropertyParser::read(QVariant::Url, cleanedValue);

        QVariant value(cleanedValue);
        if (property->asBooleanValue()) {
            value.convert(QVariant::Bool);
            return value;
        } else if (property->asNumberValue()) {
            value.convert(QVariant::Double);
            return value;
        } else if (property->asStringValue()) {
            // nothing to do
        } else { //property alias et al
            if (!hasQuotes)
                return cleverConvert(cleanedValue);
        }
        return value;
    }

    QVariant convertToEnum(AST::Statement *rhs, const QString &propertyPrefix, AST::UiQualifiedId *propertyId, const QString &astValue)
    {
        QStringList astValueList = astValue.split(QStringLiteral("."));

        if (astValueList.count() == 2) {
            //Check for global Qt enums
            if (astValueList.first() == QStringLiteral("Qt")
                    && globalQtEnums().contains(astValueList.last()))
                return QVariant::fromValue(Enumeration(astValue));

            //Check for known enum scopes used globally
            if (knownEnumScopes().contains(astValueList.first()))
                return QVariant::fromValue(Enumeration(astValue));
        }

        AST::ExpressionStatement *eStmt = AST::cast<AST::ExpressionStatement *>(rhs);
        if (!eStmt || !eStmt->expression)
            return QVariant();

        const ObjectValue *containingObject = 0;
        QString name;
        if (!lookupProperty(propertyPrefix, propertyId, 0, &containingObject, &name))
            return QVariant();

        if (containingObject)
            containingObject->lookupMember(name, m_context, &containingObject);
        const CppComponentValue * lhsCppComponent = value_cast<CppComponentValue>(containingObject);
        if (!lhsCppComponent)
            return QVariant();
        const QString lhsPropertyTypeName = lhsCppComponent->propertyType(name);

        const ObjectValue *rhsValueObject = 0;
        QString rhsValueName;
        if (AST::IdentifierExpression *idExp = AST::cast<AST::IdentifierExpression *>(eStmt->expression)) {
            if (!m_scopeChain.qmlScopeObjects().isEmpty())
                rhsValueObject = m_scopeChain.qmlScopeObjects().last();
            if (!idExp->name.isEmpty())
                rhsValueName = idExp->name.toString();
        } else if (AST::FieldMemberExpression *memberExp = AST::cast<AST::FieldMemberExpression *>(eStmt->expression)) {
            Evaluate evaluate(&m_scopeChain);
            const Value *result = evaluate(memberExp->base);
            rhsValueObject = result->asObjectValue();

            if (!memberExp->name.isEmpty())
                rhsValueName = memberExp->name.toString();
        }

        if (rhsValueObject)
            rhsValueObject->lookupMember(rhsValueName, m_context, &rhsValueObject);

        const CppComponentValue *rhsCppComponentValue = value_cast<CppComponentValue>(rhsValueObject);
        if (!rhsCppComponentValue)
            return QVariant();

        if (rhsCppComponentValue->getEnum(lhsPropertyTypeName).hasKey(rhsValueName))
            return QVariant::fromValue(Enumeration(astValue));
        else
            return QVariant();
    }


    const ScopeChain &scopeChain() const
    { return m_scopeChain; }

    QList<DiagnosticMessage> diagnosticLinkMessages() const
    { return m_diagnosticLinkMessages; }

private:
    Snapshot m_snapshot;
    Document::Ptr m_doc;
    Link m_link;
    QList<DiagnosticMessage> m_diagnosticLinkMessages;
    ContextPtr m_context;
    ScopeChain m_scopeChain;
    ScopeBuilder m_scopeBuilder;
};

} // namespace Internal
} // namespace QmlDesigner

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;


static inline bool smartVeryFuzzyCompare(QVariant value1, QVariant value2)
{ //we ignore slight changes on doubles and only check three digits
    if ((value1.type() == QVariant::Double) || (value2.type() == QVariant::Double)) {
        bool ok1, ok2;
        qreal a = value1.toDouble(&ok1);
        qreal b = value2.toDouble(&ok2);

        if (!ok1 || !ok2)
            return false;

        if (qFuzzyCompare(a, b))
            return true;

        int ai = qRound(a * 1000);
        int bi = qRound(b * 1000);

        if (qFuzzyCompare((qreal(ai) / 1000), (qreal(bi) / 1000)))
            return true;
    }
    return false;
}

static inline bool equals(const QVariant &a, const QVariant &b)
{
    if (a.canConvert<Enumeration>() && b.canConvert<Enumeration>())
        return a.value<Enumeration>().toString() == b.value<Enumeration>().toString();
    if (a == b)
        return true;
    if (smartVeryFuzzyCompare(a, b))
        return true;
    return false;
}

TextToModelMerger::TextToModelMerger(RewriterView *reWriterView) :
        m_rewriterView(reWriterView),
        m_isActive(false)
{
    Q_ASSERT(reWriterView);
    m_setupTimer.setSingleShot(true);
    RewriterView::connect(&m_setupTimer, SIGNAL(timeout()), reWriterView, SLOT(delayedSetup()));
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
    QList<Import> existingImports = m_rewriterView->model()->imports();

    for (AST::UiHeaderItemList *iter = doc->qmlProgram()->headers; iter; iter = iter->next) {
        AST::UiImport *import = AST::cast<AST::UiImport *>(iter->headerItem);
        if (!import)
            continue;

        QString version;
        if (import->versionToken.isValid())
            version = textAt(doc, import->versionToken);
        const QString &as = import->importId.toString();

        if (!import->fileName.isEmpty()) {
            const QString strippedFileName = stripQuotes(import->fileName.toString());
            const Import newImport = Import::createFileImport(strippedFileName,
                                                              version, as, m_rewriterView->importDirectories());

            if (!existingImports.removeOne(newImport))
                differenceHandler.modelMissesImport(newImport);
        } else {
            QString importUri = toString(import->importUri);
            if (importUri == QStringLiteral("Qt") && version == QStringLiteral("4.7")) {
                importUri = QStringLiteral("QtQuick");
                version = QStringLiteral("1.0");
            }

            const Import newImport =
                    Import::createLibraryImport(importUri, version, as, m_rewriterView->importDirectories());

            if (!existingImports.removeOne(newImport))
                differenceHandler.modelMissesImport(newImport);
        }
    }

    foreach (const Import &import, existingImports)
        differenceHandler.importAbsentInQMl(import);
}

static bool isLatestImportVersion(const ImportKey &importKey, const QHash<QString, ImportKey> &filteredPossibleImportKeys)
{
    return !filteredPossibleImportKeys.contains(importKey.path())
            || filteredPossibleImportKeys.value(importKey.path()).majorVersion < importKey.majorVersion
            || (filteredPossibleImportKeys.value(importKey.path()).majorVersion == importKey.majorVersion
                && filteredPossibleImportKeys.value(importKey.path()).minorVersion < importKey.minorVersion);
}

static bool isBlacklistImport(const ImportKey &importKey)
{
    QString importPathFirst = importKey.splitPath.first();
    QString importPathLast = importKey.splitPath.last();
    return importPathFirst == QStringLiteral("<cpp>")
            || importPathFirst == QStringLiteral("QML")
            || importPathFirst == QStringLiteral("QtQml")
            || (importPathFirst == QStringLiteral("QtQuick") && importPathLast == QStringLiteral("PrivateWidgets"))
            || importPathLast == QStringLiteral("Private")
            || importKey.libraryQualifiedPath() == QStringLiteral("QtQuick.Particles") //Unsupported
            || importKey.libraryQualifiedPath() == QStringLiteral("QtQuick.Dialogs")   //Unsupported
            || importKey.libraryQualifiedPath() == QStringLiteral("QtQuick.Controls.Styles")   //Unsupported
            || importKey.libraryQualifiedPath() == QStringLiteral("QtNfc") //Unsupported
            || importKey.libraryQualifiedPath() == QStringLiteral("QtMultimedia")
            || importKey.libraryQualifiedPath() == QStringLiteral("Qt.WebSockets")
            || importKey.libraryQualifiedPath() == QStringLiteral("QtWebkit")
            || importKey.libraryQualifiedPath() == QStringLiteral("QtLocation")
            || importKey.libraryQualifiedPath() == QStringLiteral("QtWebEngine")
            || importKey.libraryQualifiedPath() == QStringLiteral("QtWebChannel")
            || importKey.libraryQualifiedPath() == QStringLiteral("QtWinExtras")
            || importKey.libraryQualifiedPath() == QStringLiteral("QtPurchasing")
            || importKey.libraryQualifiedPath() == QStringLiteral("QtBluetooth")
            || importKey.libraryQualifiedPath() ==  QStringLiteral("Enginio")

            || (importKey.splitPath.count() == 1 && importPathFirst == QStringLiteral("QtQuick")); // Don't show Quick X.X imports
}

static QHash<QString, ImportKey> filterPossibleImportKeys(const QSet<ImportKey> &possibleImportKeys)
{
    QHash<QString, ImportKey> filteredPossibleImportKeys;
    foreach (const ImportKey &importKey, possibleImportKeys) {
        if (isLatestImportVersion(importKey, filteredPossibleImportKeys) && !isBlacklistImport(importKey))
            filteredPossibleImportKeys.insert(importKey.path(), importKey);
    }

    return filteredPossibleImportKeys;
}

static void removeUsedImports(QHash<QString, ImportKey> &filteredPossibleImportKeys, const QList<QmlJS::Import> &usedImports)
{
    foreach (const QmlJS::Import &import, usedImports)
        filteredPossibleImportKeys.remove(import.info.path());
}

static QList<QmlDesigner::Import> generatePossibleFileImports(const QString &path)
{
    QList<QmlDesigner::Import> possibleImports;

    foreach (const QString &subDir, QDir(path).entryList(QDir::Dirs | QDir::NoDot | QDir::NoDotDot)) {
        QDir dir(path + "/" + subDir);
        if (!dir.entryInfoList(QStringList("*.qml"), QDir::Files).isEmpty()
                && dir.entryInfoList(QStringList("qmldir"), QDir::Files).isEmpty()) {
            QmlDesigner::Import import = QmlDesigner::Import::createFileImport(subDir);
            possibleImports.append(import);
        }
    }

    return possibleImports;
}

static QList<QmlDesigner::Import> generatePossibleLibraryImports(const QHash<QString, ImportKey> &filteredPossibleImportKeys)
{
    QList<QmlDesigner::Import> possibleImports;

    foreach (const ImportKey &importKey, filteredPossibleImportKeys) {
        QString libraryName = importKey.splitPath.join(QLatin1Char('.'));
        int majorVersion = importKey.majorVersion;
        if (majorVersion >= 0) {
            int minorVersion = (importKey.minorVersion == LanguageUtils::ComponentVersion::NoVersion) ? 0 : importKey.minorVersion;
            QString version = QStringLiteral("%1.%2").arg(majorVersion).arg(minorVersion);
            possibleImports.append(QmlDesigner::Import::createLibraryImport(libraryName, version));
        }
    }

    return possibleImports;
}

void TextToModelMerger::setupPossibleImports(const QmlJS::Snapshot &snapshot, const QmlJS::ViewerContext &viewContext)
{
    QHash<QString, ImportKey> filteredPossibleImportKeys = filterPossibleImportKeys(snapshot.importDependencies()->libraryImports(viewContext));

    removeUsedImports(filteredPossibleImportKeys, m_scopeChain->context()->imports(m_document.data())->all());

    QList<QmlDesigner::Import> possibleImports = generatePossibleLibraryImports(filteredPossibleImportKeys);

    possibleImports.append(generatePossibleFileImports(document()->path()));

    if (m_rewriterView->isAttached())
        m_rewriterView->model()->setPossibleImports(possibleImports);
}

void TextToModelMerger::setupUsedImports()
{
     QList<QmlJS::Import> allImports = m_scopeChain->context()->imports(m_document.data())->all();

     QList<Import> usedImports;

     foreach (const QmlJS::Import &import, allImports) {
         if (import.used && !import.info.name().isEmpty()) {
            if (import.info.type() == ImportType::Library) {
                usedImports.append(Import::createLibraryImport(import.info.name(), import.info.version().toString(), import.info.as()));
            } else if (import.info.type() == ImportType::Directory || import.info.type() == ImportType::File) {
                usedImports.append(Import::createFileImport(import.info.name(), import.info.version().toString(), import.info.as()));
            }
         }
     }

    if (m_rewriterView->isAttached())
        m_rewriterView->model()->setUsedImports(usedImports);
}

Document::MutablePtr TextToModelMerger::createParsedDocument(const QUrl &url, const QString &data, QList<DocumentMessage> *errors)
{
    const QString fileName = url.toLocalFile();

    Dialect dialect = ModelManagerInterface::guessLanguageOfFile(fileName);
    if (dialect == Dialect::AnyLanguage
            || dialect == Dialect::NoLanguage)
        dialect = Dialect::Qml;

    Document::MutablePtr doc = Document::create(fileName.isEmpty() ? QStringLiteral("<internal>") : fileName, dialect);
    doc->setSource(data);
    doc->parseQml();

    if (!doc->isParsedCorrectly()) {
        if (errors) {
            foreach (const QmlJS::DiagnosticMessage &message, doc->diagnosticMessages())
                errors->append(DocumentMessage(message, QUrl::fromLocalFile(doc->fileName())));
        }
        return Document::MutablePtr();
    }
    return doc;
}

bool TextToModelMerger::load(const QString &data, DifferenceHandler &differenceHandler)
{
    qCInfo(rewriterBenchmark) << Q_FUNC_INFO;

    QTime time;
    if (rewriterBenchmark().isInfoEnabled())
        time.start();

    // maybe the project environment (kit, ...) changed, so we need to clean old caches
    NodeMetaInfo::clearCache();

    m_qrcMapping.clear();
    m_rewriterView->clearErrorAndWarnings();

    const QUrl url = m_rewriterView->model()->fileUrl();

    setActive(true);

    try {
        Snapshot snapshot = m_rewriterView->textModifier()->qmljsSnapshot();

        QList<DocumentMessage> errors;
        QList<DocumentMessage> warnings;
        if (Document::MutablePtr doc = createParsedDocument(url, data, &errors)) {
            snapshot.insert(doc);
            m_document = doc;
            qCInfo(rewriterBenchmark) << "parsed correctly: " << time.elapsed();
        } else {
            qCInfo(rewriterBenchmark) << "did not parse correctly: " << time.elapsed();
            m_rewriterView->setErrors(errors);
            setActive(false);
            return false;
        }
        m_vContext = ModelManagerInterface::instance()->defaultVContext(Dialect::Qml, m_document);
        ReadingContext ctxt(snapshot, m_document, m_vContext);
        m_scopeChain = QSharedPointer<const ScopeChain>(
                    new ScopeChain(ctxt.scopeChain()));

        qCInfo(rewriterBenchmark) << "linked:" << time.elapsed();
        collectLinkErrors(&errors, ctxt);

        setupImports(m_document, differenceHandler);
        setupPossibleImports(snapshot, m_vContext);

        collectImportErrors(&errors);

        if (view()->checkSemanticErrors()) {
            collectSemanticErrorsAndWarnings(&errors, &warnings);

            if (!errors.isEmpty()) {
                m_rewriterView->setErrors(errors);
                setActive(false);
                return false;
            }
            m_rewriterView->setWarnings(warnings);
            qCInfo(rewriterBenchmark) << "checked semantic errors:" << time.elapsed();
        }
        setupUsedImports();

        AST::UiObjectMember *astRootNode = 0;
        if (AST::UiProgram *program = m_document->qmlProgram())
            if (program->members)
                astRootNode = program->members->member;
        ModelNode modelRootNode = m_rewriterView->rootModelNode();
        syncNode(modelRootNode, astRootNode, &ctxt, differenceHandler);
        m_rewriterView->positionStorage()->cleanupInvalidOffsets();

        qCInfo(rewriterBenchmark) << "synced nodes:" << time.elapsed();

        setActive(false);
        return true;
    } catch (Exception &e) {
        DocumentMessage error(&e);
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
                                 AST::UiObjectMember *astNode,
                                 ReadingContext *context,
                                 DifferenceHandler &differenceHandler)
{
    AST::UiQualifiedId *astObjectType = qualifiedTypeNameId(astNode);
    AST::UiObjectInitializer *astInitializer = initializerOfObject(astNode);

    if (!astObjectType || !astInitializer)
        return;

    m_rewriterView->positionStorage()->setNodeOffset(modelNode, astObjectType->identifierToken.offset);

    QString typeNameString;
    QString defaultPropertyNameString;
    int majorVersion;
    int minorVersion;
    context->lookup(astObjectType, typeNameString, majorVersion, minorVersion, defaultPropertyNameString);

    TypeName typeName = typeNameString.toUtf8();
    PropertyName defaultPropertyName = defaultPropertyNameString.toUtf8();

    if (defaultPropertyName.isEmpty()) //fallback and use the meta system of the model
        defaultPropertyName = modelNode.metaInfo().defaultPropertyName();

    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << toString(astObjectType);
        return;
    }

    if (modelNode.isRootNode() && isComponentType(typeName)) {
        for (AST::UiObjectMemberList *iter = astInitializer->members; iter; iter = iter->next) {
            if (AST::UiObjectDefinition *def = AST::cast<AST::UiObjectDefinition *>(iter->member)) {
                syncNode(modelNode, def, context, differenceHandler);
                return;
            }
        }
    }

    bool isImplicitComponent = modelNode.hasParentProperty() && propertyIsComponentType(modelNode.parentProperty(), typeName, modelNode.model());


    if (modelNode.type() != typeName //If there is no valid parentProperty                                                                                                      //the node has just been created. The type is correct then.
            || modelNode.majorVersion() != majorVersion
            || modelNode.minorVersion() != minorVersion) {
        const bool isRootNode = m_rewriterView->rootModelNode() == modelNode;
        differenceHandler.typeDiffers(isRootNode, modelNode, typeName,
                                      majorVersion, minorVersion,
                                      astNode, context);
        if (!isRootNode)
            return; // the difference handler will create a new node, so we're done.
    }

    if (isComponentType(typeName) || isImplicitComponent)
        setupComponentDelayed(modelNode, differenceHandler.isValidator());

    if (isCustomParserType(typeName))
        setupCustomParserNodeDelayed(modelNode, differenceHandler.isValidator());

    context->enterScope(astNode);

    QSet<PropertyName> modelPropertyNames = QSet<PropertyName>::fromList(modelNode.propertyNames());
    if (!modelNode.id().isEmpty())
        modelPropertyNames.insert("id");
    QList<AST::UiObjectMember *> defaultPropertyItems;

    for (AST::UiObjectMemberList *iter = astInitializer->members; iter; iter = iter->next) {
        AST::UiObjectMember *member = iter->member;
        if (!member)
            continue;

        if (AST::UiArrayBinding *array = AST::cast<AST::UiArrayBinding *>(member)) {
            const QString astPropertyName = toString(array->qualifiedId);
            if (isPropertyChangesType(typeName) || isConnectionsType(typeName) || context->lookupProperty(QString(), array->qualifiedId)) {
                AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
                QList<AST::UiObjectMember *> arrayMembers;
                for (AST::UiArrayMemberList *iter = array->members; iter; iter = iter->next)
                    if (AST::UiObjectMember *member = iter->member)
                        arrayMembers.append(member);

                syncArrayProperty(modelProperty, arrayMembers, context, differenceHandler);
                modelPropertyNames.remove(astPropertyName.toUtf8());
            } else {
                qWarning() << "Skipping invalid array property" << astPropertyName
                           << "for node type" << modelNode.type();
            }
        } else if (AST::UiObjectDefinition *def = AST::cast<AST::UiObjectDefinition *>(member)) {
            const QString &name = def->qualifiedTypeNameId->name.toString();
            if (name.isEmpty() || !name.at(0).isUpper()) {
                QStringList props = syncGroupedProperties(modelNode,
                                                          name,
                                                          def->initializer->members,
                                                          context,
                                                          differenceHandler);
                foreach (const QString &prop, props)
                    modelPropertyNames.remove(prop.toUtf8());
            } else {
                defaultPropertyItems.append(member);
            }
        } else if (AST::UiObjectBinding *binding = AST::cast<AST::UiObjectBinding *>(member)) {
            const QString astPropertyName = toString(binding->qualifiedId);
            if (binding->hasOnToken) {
                // skip value sources
            } else {
                const Value *propertyType = 0;
                const ObjectValue *containingObject = 0;
                QString name;
                if (context->lookupProperty(QString(), binding->qualifiedId, &propertyType, &containingObject, &name)
                        || isPropertyChangesType(typeName)
                        || isConnectionsType(typeName)) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
                    if (context->isArrayProperty(propertyType, containingObject, name))
                        syncArrayProperty(modelProperty, QList<AST::UiObjectMember*>() << member, context, differenceHandler);
                    else
                        syncNodeProperty(modelProperty, binding, context, TypeName(), differenceHandler);
                    modelPropertyNames.remove(astPropertyName.toUtf8());
                } else {
                    qWarning() << "Syncing unknown node property" << astPropertyName
                               << "for node type" << modelNode.type();
                    AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
                    syncNodeProperty(modelProperty, binding, context, TypeName(), differenceHandler);
                    modelPropertyNames.remove(astPropertyName.toUtf8());
                }
            }
        } else if (AST::UiScriptBinding *script = AST::cast<AST::UiScriptBinding *>(member)) {
            modelPropertyNames.remove(syncScriptBinding(modelNode, QString(), script, context, differenceHandler));
        } else if (AST::UiPublicMember *property = AST::cast<AST::UiPublicMember *>(member)) {
            if (property->type == AST::UiPublicMember::Signal)
                continue; // QML designer doesn't support this yet.

            if (property->name.isEmpty() || property->memberType.isEmpty())
                continue; // better safe than sorry.

            const QStringRef astName = property->name;
            QString astValue;
            if (property->statement)
                astValue = textAt(context->doc(),
                                  property->statement->firstSourceLocation(),
                                  property->statement->lastSourceLocation());

            astValue = astValue.trimmed();
            if (astValue.endsWith(QLatin1Char(';')))
                astValue = astValue.left(astValue.length() - 1);
            astValue = astValue.trimmed();

            const TypeName &astType = property->memberType.toUtf8();
            AbstractProperty modelProperty = modelNode.property(astName.toUtf8());

            if (property->binding) {
                if (AST::UiObjectBinding *binding = AST::cast<AST::UiObjectBinding *>(property->binding))
                    syncNodeProperty(modelProperty, binding, context, astType, differenceHandler);
                else
                    qWarning() << "Arrays are not yet supported";
            } else if (!property->statement || isLiteralValue(property->statement)) {
                const QVariant variantValue = convertDynamicPropertyValueToVariant(astValue, QString::fromLatin1(astType));
                syncVariantProperty(modelProperty, variantValue, astType, differenceHandler);
            } else {
                syncExpressionProperty(modelProperty, astValue, astType, differenceHandler);
            }
            modelPropertyNames.remove(astName.toUtf8());
        } else {
            qWarning() << "Found an unknown QML value.";
        }
    }

    if (!defaultPropertyItems.isEmpty()) {
        if (isComponentType(modelNode.type()))
            setupComponentDelayed(modelNode, differenceHandler.isValidator());
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

    foreach (const PropertyName &modelPropertyName, modelPropertyNames) {
        AbstractProperty modelProperty = modelNode.property(modelPropertyName);

        // property deleted.
        if (modelPropertyName == "id")
            differenceHandler.idsDiffer(modelNode, QString());
        else
            differenceHandler.propertyAbsentFromQml(modelProperty);
    }

    context->leaveScope();
}

static QVariant parsePropertyExpression(AST::ExpressionNode *expressionNode)
{
    Q_ASSERT(expressionNode);

    AST::ArrayLiteral *arrayLiteral = AST::cast<AST::ArrayLiteral *>(expressionNode);

    if (arrayLiteral) {
        QList<QVariant> variantList;
        for (AST::ElementList *it = arrayLiteral->elements; it; it = it->next)
            variantList << parsePropertyExpression(it->expression);
        return variantList;
    }

    AST::StringLiteral *stringLiteral = AST::cast<AST::StringLiteral *>(expressionNode);
    if (stringLiteral)
        return stringLiteral->value.toString();

    AST::TrueLiteral *trueLiteral = AST::cast<AST::TrueLiteral *>(expressionNode);
    if (trueLiteral)
        return true;

    AST::FalseLiteral *falseLiteral = AST::cast<AST::FalseLiteral *>(expressionNode);
    if (falseLiteral)
        return false;

    AST::NumericLiteral *numericLiteral = AST::cast<AST::NumericLiteral *>(expressionNode);
    if (numericLiteral)
        return numericLiteral->value;


    return QVariant();
}

QVariant parsePropertyScriptBinding(AST::UiScriptBinding *uiScriptBinding)
{
    Q_ASSERT(uiScriptBinding);

    AST::ExpressionStatement *expStmt = AST::cast<AST::ExpressionStatement *>(uiScriptBinding->statement);
    if (!expStmt)
        return QVariant();

    return parsePropertyExpression(expStmt->expression);
}

QmlDesigner::PropertyName TextToModelMerger::syncScriptBinding(ModelNode &modelNode,
                                             const QString &prefix,
                                             AST::UiScriptBinding *script,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    QString astPropertyName = toString(script->qualifiedId);
    if (!prefix.isEmpty())
        astPropertyName.prepend(prefix + '.');

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

    if (astPropertyName == QStringLiteral("id")) {
        syncNodeId(modelNode, astValue, differenceHandler);
        return astPropertyName.toUtf8();
    }

    if (isSignalPropertyName(astPropertyName)) {
        AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
        syncSignalHandler(modelProperty, astValue, differenceHandler);
        return astPropertyName.toUtf8();
    }

    if (isLiteralValue(script)) {
        if (isPropertyChangesType(modelNode.type())
                || isConnectionsType(modelNode.type())
                || isListElementType(modelNode.type())) {
            AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
            QVariant variantValue = parsePropertyScriptBinding(script);
            if (!variantValue.isValid())
                variantValue = deEscape(stripQuotes(astValue));
            syncVariantProperty(modelProperty, variantValue, TypeName(), differenceHandler);
            return astPropertyName.toUtf8();
        } else {
            const QVariant variantValue = context->convertToVariant(astValue, prefix, script->qualifiedId);
            if (variantValue.isValid()) {
                AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
                syncVariantProperty(modelProperty, variantValue, TypeName(), differenceHandler);
                return astPropertyName.toUtf8();
            } else {
                qWarning() << "Skipping invalid variant property" << astPropertyName
                           << "for node type" << modelNode.type();
                return PropertyName();
            }
        }
    }

    const QVariant enumValue = context->convertToEnum(script->statement, prefix, script->qualifiedId, astValue);
    if (enumValue.isValid()) { // It is a qualified enum:
        AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
        syncVariantProperty(modelProperty, enumValue, TypeName(), differenceHandler); // TODO: parse type
        return astPropertyName.toUtf8();
    } else { // Not an enum, so:
        if (isPropertyChangesType(modelNode.type())
                || isConnectionsType(modelNode.type())
                || context->lookupProperty(prefix, script->qualifiedId)
                || isSupportedAttachedProperties(astPropertyName)) {
            AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
            syncExpressionProperty(modelProperty, astValue, TypeName(), differenceHandler); // TODO: parse type
            return astPropertyName.toUtf8();
        } else {
            qWarning() << Q_FUNC_INFO << "Skipping invalid expression property" << astPropertyName
                    << "for node type" << modelNode.type();
            return PropertyName();
        }
    }
}

void TextToModelMerger::syncNodeId(ModelNode &modelNode, const QString &astObjectId,
                                   DifferenceHandler &differenceHandler)
{
    if (astObjectId.isEmpty()) {
        if (!modelNode.id().isEmpty()) {
            ModelNode existingNodeWithId = m_rewriterView->modelNodeForId(astObjectId);
            if (existingNodeWithId.isValid())
                existingNodeWithId.setIdWithoutRefactoring(QString());
            differenceHandler.idsDiffer(modelNode, astObjectId);
        }
    } else {
        if (modelNode.id() != astObjectId) {
            ModelNode existingNodeWithId = m_rewriterView->modelNodeForId(astObjectId);
            if (existingNodeWithId.isValid())
                existingNodeWithId.setIdWithoutRefactoring(QString());
            differenceHandler.idsDiffer(modelNode, astObjectId);
        }
    }
}

void TextToModelMerger::syncNodeProperty(AbstractProperty &modelProperty,
                                         AST::UiObjectBinding *binding,
                                         ReadingContext *context,
                                         const TypeName &dynamicPropertyType,
                                         DifferenceHandler &differenceHandler)
{

    QString typeNameString;
    QString dummy;
    int majorVersion;
    int minorVersion;
    context->lookup(binding->qualifiedTypeNameId, typeNameString, majorVersion, minorVersion, dummy);

    TypeName typeName = typeNameString.toUtf8();


    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << toString(binding->qualifiedTypeNameId);
        return;
    }

    if (modelProperty.isNodeProperty() && dynamicPropertyType == modelProperty.dynamicTypeName()) {
        ModelNode nodePropertyNode = modelProperty.toNodeProperty().modelNode();
        syncNode(nodePropertyNode, binding, context, differenceHandler);
    } else {
        differenceHandler.shouldBeNodeProperty(modelProperty,
                                               typeName,
                                               majorVersion,
                                               minorVersion,
                                               binding,
                                               dynamicPropertyType,
                                               context);
    }
}

void TextToModelMerger::syncExpressionProperty(AbstractProperty &modelProperty,
                                               const QString &javascript,
                                               const TypeName &astType,
                                               DifferenceHandler &differenceHandler)
{
    if (modelProperty.isBindingProperty()) {
        BindingProperty bindingProperty = modelProperty.toBindingProperty();
        if (bindingProperty.expression() != javascript
                || !astType.isEmpty() != bindingProperty.isDynamic()
                || astType != bindingProperty.dynamicTypeName()) {
            differenceHandler.bindingExpressionsDiffer(bindingProperty, javascript, astType);
        }
    } else {
        differenceHandler.shouldBeBindingProperty(modelProperty, javascript, astType);
    }
}

void TextToModelMerger::syncSignalHandler(AbstractProperty &modelProperty,
                                               const QString &javascript,
                                               DifferenceHandler &differenceHandler)
{
    if (modelProperty.isSignalHandlerProperty()) {
        SignalHandlerProperty signalHandlerProperty = modelProperty.toSignalHandlerProperty();
        if (signalHandlerProperty.source() != javascript)
            differenceHandler.signalHandlerSourceDiffer(signalHandlerProperty, javascript);
    } else {
        differenceHandler.shouldBeSignalHandlerProperty(modelProperty, javascript);
    }
}


void TextToModelMerger::syncArrayProperty(AbstractProperty &modelProperty,
                                          const QList<AST::UiObjectMember *> &arrayMembers,
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

static QString fileForFullQrcPath(const QString &string)
{
    QStringList stringList = string.split(QLatin1String("/"));
    if (stringList.isEmpty())
        return QString();

    return stringList.last();
}

static QString removeFileFromQrcPath(const QString &string)
{
    QStringList stringList = string.split(QLatin1String("/"));
    if (stringList.isEmpty())
        return QString();

    stringList.removeLast();
    return stringList.join(QLatin1String("/"));
}

void TextToModelMerger::syncVariantProperty(AbstractProperty &modelProperty,
                                            const QVariant &astValue,
                                            const TypeName &astType,
                                            DifferenceHandler &differenceHandler)
{
    if (astValue.canConvert(QMetaType::QString))
        populateQrcMapping(astValue.toString());

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
                                             const QList<AST::UiObjectMember *> arrayMembers,
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
        AST::UiObjectMember *arrayMember = arrayMembers.at(j);
        const ModelNode newNode = differenceHandler.listPropertyMissingModelNode(modelListProperty, context, arrayMember);
    }

    for (int j = i; j < modelNodes.size(); ++j) {
        // more elements in the model, so remove them.
        ModelNode modelNode = modelNodes.at(j);
        differenceHandler.modelNodeAbsentFromQml(modelNode);
    }
}

ModelNode TextToModelMerger::createModelNode(const TypeName &typeName,
                                             int majorVersion,
                                             int minorVersion,
                                             bool isImplicitComponent,
                                             AST::UiObjectMember *astNode,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    QString nodeSource;


    AST::UiQualifiedId *astObjectType = qualifiedTypeNameId(astNode);

    if (isCustomParserType(typeName))
        nodeSource = textAt(context->doc(),
                                    astObjectType->identifierToken,
                                    astNode->lastSourceLocation());


    if (isComponentType(typeName) || isImplicitComponent) {
        QString componentSource = extractComponentFromQml(textAt(context->doc(),
                                  astObjectType->identifierToken,
                                  astNode->lastSourceLocation()));


        nodeSource = componentSource;
    }

    ModelNode::NodeSourceType nodeSourceType = ModelNode::NodeWithoutSource;

    if (isComponentType(typeName) || isImplicitComponent)
        nodeSourceType = ModelNode::NodeWithComponentSource;
    else if (isCustomParserType(typeName))
        nodeSourceType = ModelNode::NodeWithCustomParserSource;

    ModelNode newNode = m_rewriterView->createModelNode(typeName,
                                                        majorVersion,
                                                        minorVersion,
                                                        PropertyListType(),
                                                        PropertyListType(),
                                                        nodeSource,
                                                        nodeSourceType);

    syncNode(newNode, astNode, context, differenceHandler);
    return newNode;
}

QStringList TextToModelMerger::syncGroupedProperties(ModelNode &modelNode,
                                                     const QString &name,
                                                     AST::UiObjectMemberList *members,
                                                     ReadingContext *context,
                                                     DifferenceHandler &differenceHandler)
{
    QStringList props;

    for (AST::UiObjectMemberList *iter = members; iter; iter = iter->next) {
        AST::UiObjectMember *member = iter->member;

        if (AST::UiScriptBinding *script = AST::cast<AST::UiScriptBinding *>(member)) {
            const QString prop = QString::fromLatin1(syncScriptBinding(modelNode, name, script, context, differenceHandler));
            if (!prop.isEmpty())
                props.append(prop);
        }
    }

    return props;
}

void ModelValidator::modelMissesImport(const QmlDesigner::Import &import)
{
    Q_UNUSED(import)
    Q_ASSERT(m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::importAbsentInQMl(const QmlDesigner::Import &import)
{
    Q_UNUSED(import)
    Q_ASSERT(! m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::bindingExpressionsDiffer(BindingProperty &modelProperty,
                                              const QString &javascript,
                                              const TypeName &astType)
{
    Q_UNUSED(modelProperty)
    Q_UNUSED(javascript)
    Q_UNUSED(astType)
    Q_ASSERT(modelProperty.expression() == javascript);
    Q_ASSERT(modelProperty.dynamicTypeName() == astType);
    Q_ASSERT(0);
}

void ModelValidator::shouldBeBindingProperty(AbstractProperty &modelProperty,
                                             const QString &/*javascript*/,
                                             const TypeName &/*astType*/)
{
    Q_UNUSED(modelProperty)
    Q_ASSERT(modelProperty.isBindingProperty());
    Q_ASSERT(0);
}

void ModelValidator::signalHandlerSourceDiffer(SignalHandlerProperty &modelProperty, const QString &javascript)
{
    Q_UNUSED(modelProperty)
    Q_UNUSED(javascript)
    QTC_ASSERT(modelProperty.source() == javascript, return);
}

void ModelValidator::shouldBeSignalHandlerProperty(AbstractProperty &modelProperty, const QString & /*javascript*/)
{
    Q_UNUSED(modelProperty)
    Q_ASSERT(modelProperty.isSignalHandlerProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                              const QList<AST::UiObjectMember *> /*arrayMembers*/,
                                              ReadingContext * /*context*/)
{
    Q_UNUSED(modelProperty)
    Q_ASSERT(modelProperty.isNodeListProperty());
    Q_ASSERT(0);
}

void ModelValidator::variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicTypeName)
{
    Q_UNUSED(modelProperty)
    Q_UNUSED(qmlVariantValue)
    Q_UNUSED(dynamicTypeName)

    QTC_ASSERT(modelProperty.isDynamic() == !dynamicTypeName.isEmpty(), return);
    if (modelProperty.isDynamic()) {
        QTC_ASSERT(modelProperty.dynamicTypeName() == dynamicTypeName, return);
    }



    QTC_ASSERT(equals(modelProperty.value(), qmlVariantValue), qWarning() << modelProperty.value() << qmlVariantValue);
    QTC_ASSERT(0, return);
}

void ModelValidator::shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &/*qmlVariantValue*/, const TypeName &/*dynamicTypeName*/)
{
    Q_UNUSED(modelProperty)

    Q_ASSERT(modelProperty.isVariantProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeProperty(AbstractProperty &modelProperty,
                                          const TypeName &/*typeName*/,
                                          int /*majorVersion*/,
                                          int /*minorVersion*/,
                                          AST::UiObjectMember * /*astNode*/,
                                          const TypeName & /*dynamicPropertyType */,
                                          ReadingContext * /*context*/)
{
    Q_UNUSED(modelProperty)

    Q_ASSERT(modelProperty.isNodeProperty());
    Q_ASSERT(0);
}

void ModelValidator::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    Q_UNUSED(modelNode)

    Q_ASSERT(!modelNode.isValid());
    Q_ASSERT(0);
}

ModelNode ModelValidator::listPropertyMissingModelNode(NodeListProperty &/*modelProperty*/,
                                                       ReadingContext * /*context*/,
                                                       AST::UiObjectMember * /*arrayMember*/)
{
    Q_ASSERT(0);
    return ModelNode();
}

void ModelValidator::typeDiffers(bool /*isRootNode*/,
                                 ModelNode &modelNode,
                                 const TypeName &typeName,
                                 int majorVersion,
                                 int minorVersion,
                                 QmlJS::AST::UiObjectMember * /*astNode*/,
                                 ReadingContext * /*context*/)
{
    Q_UNUSED(modelNode)
    Q_UNUSED(typeName)
    Q_UNUSED(minorVersion)
    Q_UNUSED(majorVersion)

    QTC_ASSERT(modelNode.type() == typeName, return);

    if (modelNode.majorVersion() != majorVersion) {
        qDebug() << Q_FUNC_INFO << modelNode;
        qDebug() << typeName << modelNode.majorVersion() << majorVersion;
    }

    QTC_ASSERT(modelNode.majorVersion() == majorVersion, return);
    QTC_ASSERT(modelNode.minorVersion() == minorVersion, return);
    QTC_ASSERT(0, return);
}

void ModelValidator::propertyAbsentFromQml(AbstractProperty &modelProperty)
{
    Q_UNUSED(modelProperty)

    Q_ASSERT(!modelProperty.isValid());
    Q_ASSERT(0);
}

void ModelValidator::idsDiffer(ModelNode &modelNode, const QString &qmlId)
{
    Q_UNUSED(modelNode)
    Q_UNUSED(qmlId)

    QTC_ASSERT(modelNode.id() == qmlId, return);
    QTC_ASSERT(0, return);
}

void ModelAmender::modelMissesImport(const QmlDesigner::Import &import)
{
    m_merger->view()->model()->changeImports(QList<QmlDesigner::Import>() << import, QList<QmlDesigner::Import>());
}

void ModelAmender::importAbsentInQMl(const QmlDesigner::Import &import)
{
    m_merger->view()->model()->changeImports(QList<Import>(), QList<Import>() << import);
}

void ModelAmender::bindingExpressionsDiffer(BindingProperty &modelProperty,
                                            const QString &javascript,
                                            const TypeName &astType)
{
    if (astType.isEmpty())
        modelProperty.setExpression(javascript);
    else
        modelProperty.setDynamicTypeNameAndExpression(astType, javascript);
}

void ModelAmender::shouldBeBindingProperty(AbstractProperty &modelProperty,
                                           const QString &javascript,
                                           const TypeName &astType)
{
    ModelNode theNode = modelProperty.parentModelNode();
    BindingProperty newModelProperty = theNode.bindingProperty(modelProperty.name());
    if (astType.isEmpty())
        newModelProperty.setExpression(javascript);
    else
        newModelProperty.setDynamicTypeNameAndExpression(astType, javascript);
}

void ModelAmender::signalHandlerSourceDiffer(SignalHandlerProperty &modelProperty, const QString &javascript)
{
    modelProperty.setSource(javascript);
}

void ModelAmender::shouldBeSignalHandlerProperty(AbstractProperty &modelProperty, const QString &javascript)
{
    ModelNode theNode = modelProperty.parentModelNode();
    SignalHandlerProperty newModelProperty = theNode.signalHandlerProperty(modelProperty.name());
    newModelProperty.setSource(javascript);
}

void ModelAmender::shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                            const QList<AST::UiObjectMember *> arrayMembers,
                                            ReadingContext *context)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeListProperty newNodeListProperty = theNode.nodeListProperty(modelProperty.name());
    m_merger->syncNodeListProperty(newNodeListProperty,
                                   arrayMembers,
                                   context,
                                   *this);
}



void ModelAmender::variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicType)
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

void ModelAmender::shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicTypeName)
{
    ModelNode theNode = modelProperty.parentModelNode();
    VariantProperty newModelProperty = theNode.variantProperty(modelProperty.name());

    if (dynamicTypeName.isEmpty())
        newModelProperty.setValue(qmlVariantValue);
    else
        newModelProperty.setDynamicTypeNameAndValue(dynamicTypeName, qmlVariantValue);
}

void ModelAmender::shouldBeNodeProperty(AbstractProperty &modelProperty,
                                        const TypeName &typeName,
                                        int majorVersion,
                                        int minorVersion,
                                        AST::UiObjectMember *astNode,
                                        const TypeName &dynamicPropertyType,
                                        ReadingContext *context)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeProperty newNodeProperty = theNode.nodeProperty(modelProperty.name());

    const bool propertyTakesComponent = propertyIsComponentType(newNodeProperty, typeName, theNode.model());

    const ModelNode &newNode = m_merger->createModelNode(typeName,
                                                          majorVersion,
                                                          minorVersion,
                                                          propertyTakesComponent,
                                                          astNode,
                                                          context,
                                                          *this);

    if (dynamicPropertyType.isEmpty())
        newNodeProperty.setModelNode(newNode);
    else
        newNodeProperty.setDynamicTypeNameAndsetModelNode(dynamicPropertyType, newNode);

    if (propertyTakesComponent)
        m_merger->setupComponentDelayed(newNode, true);

}

void ModelAmender::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    modelNode.destroy();
}

ModelNode ModelAmender::listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                                     ReadingContext *context,
                                                     AST::UiObjectMember *arrayMember)
{
    AST::UiQualifiedId *astObjectType = 0;
    AST::UiObjectInitializer *astInitializer = 0;
    if (AST::UiObjectDefinition *def = AST::cast<AST::UiObjectDefinition *>(arrayMember)) {
        astObjectType = def->qualifiedTypeNameId;
        astInitializer = def->initializer;
    } else if (AST::UiObjectBinding *bin = AST::cast<AST::UiObjectBinding *>(arrayMember)) {
        astObjectType = bin->qualifiedTypeNameId;
        astInitializer = bin->initializer;
    }

    if (!astObjectType || !astInitializer)
        return ModelNode();

    QString typeNameString;
    QString dummy;
    int majorVersion;
    int minorVersion;
    context->lookup(astObjectType, typeNameString, majorVersion, minorVersion, dummy);

    TypeName typeName = typeNameString.toUtf8();

    if (typeName.isEmpty()) {
        qWarning() << "Skipping node with unknown type" << toString(astObjectType);
        return ModelNode();
    }

    const bool propertyTakesComponent = propertyIsComponentType(modelProperty, typeName, m_merger->view()->model());


    const ModelNode &newNode = m_merger->createModelNode(typeName,
                                                         majorVersion,
                                                         minorVersion,
                                                         propertyTakesComponent,
                                                         arrayMember,
                                                         context,
                                                         *this);


    if (propertyTakesComponent)
        m_merger->setupComponentDelayed(newNode, true);

    if (modelProperty.isDefaultProperty() || isComponentType(modelProperty.parentModelNode().type())) { //In the default property case we do some magic
        if (modelProperty.isNodeListProperty()) {
            modelProperty.reparentHere(newNode);
        } else { //The default property could a NodeProperty implicitly (delegate:)
            modelProperty.parentModelNode().removeProperty(modelProperty.name());
            modelProperty.reparentHere(newNode);
        }
    } else {
        modelProperty.reparentHere(newNode);
    }
    return newNode;
}

void ModelAmender::typeDiffers(bool isRootNode,
                               ModelNode &modelNode,
                               const TypeName &typeName,
                               int majorVersion,
                               int minorVersion,
                               AST::UiObjectMember *astNode,
                               ReadingContext *context)
{
    const bool propertyTakesComponent = modelNode.hasParentProperty() && propertyIsComponentType(modelNode.parentProperty(), typeName, modelNode.model());

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
                                                             propertyTakesComponent,
                                                             astNode,
                                                             context,
                                                             *this);
        parentProperty.reparentHere(newNode);
        if (parentProperty.isNodeListProperty()) {
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
    modelNode.setIdWithoutRefactoring(qmlId);
}

void TextToModelMerger::setupComponent(const ModelNode &node)
{
    if (!node.isValid())
        return;

    QString componentText = m_rewriterView->extractText(QList<ModelNode>() << node).value(node);

    if (componentText.isEmpty())
        return;

    QString result = extractComponentFromQml(componentText);

    if (result.isEmpty())
        return; //No object definition found

    if (node.nodeSource() != result)
        ModelNode(node).setNodeSource(result);
}

void TextToModelMerger::collectLinkErrors(QList<DocumentMessage> *errors, const ReadingContext &ctxt)
{
    foreach (const QmlJS::DiagnosticMessage &diagnosticMessage, ctxt.diagnosticLinkMessages()) {
        errors->append(DocumentMessage(diagnosticMessage, QUrl::fromLocalFile(m_document->fileName())));
    }
}

void TextToModelMerger::collectImportErrors(QList<DocumentMessage> *errors)
{
    if (m_rewriterView->model()->imports().isEmpty()) {
        const QmlJS::DiagnosticMessage diagnosticMessage(QmlJS::Severity::Error, AST::SourceLocation(0, 0, 0, 0), QCoreApplication::translate("QmlDesigner::TextToModelMerger", "No import statements found"));
        errors->append(DocumentMessage(diagnosticMessage, QUrl::fromLocalFile(m_document->fileName())));
    }

    bool hasQtQuick = false;
    foreach (const QmlDesigner::Import &import, m_rewriterView->model()->imports()) {
        if (import.isLibraryImport() && import.url() == QStringLiteral("QtQuick")) {

            if (supportedQtQuickVersion(import.version())) {
                hasQtQuick = true;
            } else {
                const QmlJS::DiagnosticMessage diagnosticMessage(QmlJS::Severity::Error, AST::SourceLocation(0, 0, 0, 0),
                                                                 QCoreApplication::translate("QmlDesigner::TextToModelMerger", "Unsupported QtQuick version"));
                errors->append(DocumentMessage(diagnosticMessage, QUrl::fromLocalFile(m_document->fileName())));
            }
        }
    }

    if (!hasQtQuick)
        errors->append(DocumentMessage(QCoreApplication::translate("QmlDesigner::TextToModelMerger", "No import for Qt Quick found.")));
}

void TextToModelMerger::collectSemanticErrorsAndWarnings(QList<DocumentMessage> *errors, QList<DocumentMessage> *warnings)
{
    Check check(m_document, m_scopeChain->context());
    check.disableMessage(StaticAnalysis::ErrPrototypeCycle);
    check.disableMessage(StaticAnalysis::ErrCouldNotResolvePrototype);
    check.disableMessage(StaticAnalysis::ErrCouldNotResolvePrototypeOf);

    foreach (StaticAnalysis::Type type, StaticAnalysis::Message::allMessageTypes()) {
        StaticAnalysis::PrototypeMessageData prototypeMessageData = StaticAnalysis::Message::prototypeForMessageType(type);
        if (prototypeMessageData.severity == Severity::MaybeWarning
                || prototypeMessageData.severity == Severity::Warning) {
            check.disableMessage(type);
        }
    }

    check.enableQmlDesignerChecks();

    QUrl fileNameUrl = QUrl::fromLocalFile(m_document->fileName());
    foreach (const StaticAnalysis::Message &message, check()) {
        if (message.severity == Severity::Error) {
            if (message.type == StaticAnalysis::ErrUnknownComponent)
                warnings->append(DocumentMessage(message.toDiagnosticMessage(), fileNameUrl));
            else
                errors->append(DocumentMessage(message.toDiagnosticMessage(), fileNameUrl));
        }
        if (message.severity == Severity::Warning) {
            if (message.type == StaticAnalysis::WarnAboutQtQuick1InsteadQtQuick2)
                errors->append(DocumentMessage(message.toDiagnosticMessage(), fileNameUrl));
            else
                warnings->append(DocumentMessage(message.toDiagnosticMessage(), fileNameUrl));
        }
    }
}

void TextToModelMerger::populateQrcMapping(const QString &filePath)
{
    if (!filePath.startsWith(QLatin1String("qrc:")))
        return;

    QString path = removeFileFromQrcPath(filePath);
    const QString fileName = fileForFullQrcPath(filePath);
    path.remove(QLatin1String("qrc:"));
    QMap<QString,QStringList> map = ModelManagerInterface::instance()->filesInQrcPath(path);
    const QStringList qrcFilePathes = map.value(fileName, QStringList());
    if (!qrcFilePathes.isEmpty()) {
        QString fileSystemPath =  qrcFilePathes.first();
        fileSystemPath.remove(fileName);
        if (path.isEmpty())
            path.prepend(QLatin1String("/"));
        m_qrcMapping.insert(qMakePair(path, fileSystemPath));
    }
}

void TextToModelMerger::setupComponentDelayed(const ModelNode &node, bool synchron)
{
    if (synchron) {
        setupComponent(node);
    } else {
        m_setupComponentList.insert(node);
        m_setupTimer.start();
    }
}

void TextToModelMerger::setupCustomParserNode(const ModelNode &node)
{
    if (!node.isValid())
        return;

    QString modelText = m_rewriterView->extractText(QList<ModelNode>() << node).value(node);

    if (modelText.isEmpty())
        return;

    if (node.nodeSource() != modelText)
        ModelNode(node).setNodeSource(modelText);

}

void TextToModelMerger::setupCustomParserNodeDelayed(const ModelNode &node, bool synchron)
{
    Q_ASSERT(isCustomParserType(node.type()));

    if (synchron) {
        setupCustomParserNode(node);
    } else {
        m_setupCustomParserList.insert(node);
        m_setupTimer.start();
    }
}

void TextToModelMerger::delayedSetup()
{
    foreach (const ModelNode node, m_setupComponentList)
        setupComponent(node);

    foreach (const ModelNode node, m_setupCustomParserList)
        setupCustomParserNode(node);
    m_setupCustomParserList.clear();
    m_setupComponentList.clear();
}

QSet<QPair<QString, QString> > TextToModelMerger::qrcMapping() const
{
    return m_qrcMapping;
}

QString TextToModelMerger::textAt(const Document::Ptr &doc,
                                  const AST::SourceLocation &location)
{
    return doc->source().mid(location.offset, location.length);
}

QString TextToModelMerger::textAt(const Document::Ptr &doc,
                                  const AST::SourceLocation &from,
                                  const AST::SourceLocation &to)
{
    return doc->source().mid(from.offset, to.end() - from.begin());
}
