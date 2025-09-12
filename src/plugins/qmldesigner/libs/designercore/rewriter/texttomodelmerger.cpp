// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texttomodelmerger.h"

#include "abstractproperty.h"
#include "bindingproperty.h"
#include "designercoretr.h"
#include "documentmessage.h"
#include "filemanager/firstdefinitionfinder.h"
#include "filemanager/objectlengthcalculator.h"
#include "filemanager/qmlrefactoring.h"
#include "modelnodepositionstorage.h"
#include "modelutils.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "propertyparser.h"
#include "rewritertracing.h"
#include "rewriterview.h"
#include "signalhandlerproperty.h"
#include "variantproperty.h"
#include <astcheck/astcheck.h>

#include <externaldependenciesinterface.h>
#include <import.h>
#include <modelutils.h>
#include <qmldesignerutils/stringutils.h>
#include <rewritingexception.h>

#include <enumeration.h>

#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljslink.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsutils.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsvalueowner.h>

#include <utils/algorithm.h>
#include <utils/array.h>
#include <utils/qrcparser.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSet>

#include <memory>
#include <tuple>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace Qt::StringLiterals;

static Q_LOGGING_CATEGORY(rewriterBenchmark, "qtc.rewriter.load", QtWarningMsg)
static Q_LOGGING_CATEGORY(texttomodelMergerLog, "qtc.texttomodelmerger", QtWarningMsg)

namespace {

bool isSupportedAttachedProperties(const QString &propertyName)
{
    return propertyName.startsWith(QLatin1String("Layout."))
           || propertyName.startsWith(QLatin1String("InsightCategory."));
}

bool isGlobalQtEnums(QStringView value)
{
    static constexpr auto list = Utils::to_array<std::u16string_view>(u"AlignBaseline",
                                                                      u"AlignBottom",
                                                                      u"AlignHCenter",
                                                                      u"AlignLeft",
                                                                      u"AlignRight",
                                                                      u"AlignTop",
                                                                      u"AlignVCenter",
                                                                      u"AllButtons",
                                                                      u"ArrowCursor",
                                                                      u"BackButton",
                                                                      u"BlankCursor",
                                                                      u"BottomEdge",
                                                                      u"BottomLeft",
                                                                      u"BusyCursor",
                                                                      u"ClickFocus",
                                                                      u"ClosedHandCursor",
                                                                      u"CrossCursor",
                                                                      u"DragCopyCursor",
                                                                      u"DragLinkCursor",
                                                                      u"DragMoveCursor",
                                                                      u"ForbiddenCursor",
                                                                      u"ForwardButton",
                                                                      u"Horizontal",
                                                                      u"IBeamCursor",
                                                                      u"LeftButton",
                                                                      u"LeftEdge",
                                                                      u"LeftToRight",
                                                                      u"MiddleButton",
                                                                      u"NoFocus",
                                                                      u"OpenHandCursor",
                                                                      u"PointingHandCursor",
                                                                      u"RightButton",
                                                                      u"RightEdge",
                                                                      u"RightToLeft",
                                                                      u"SizeAllCursor",
                                                                      u"SizeBDiagCursor",
                                                                      u"SizeFDiagCursor",
                                                                      u"SizeHorCursor",
                                                                      u"SizeVerCursor",
                                                                      u"SplitHCursor",
                                                                      u"SplitVCursor",
                                                                      u"StrongFocus",
                                                                      u"TabFocus",
                                                                      u"TopEdge",
                                                                      u"TopToBottom",
                                                                      u"UpArrowCursor",
                                                                      u"Vertical",
                                                                      u"WaitCursor",
                                                                      u"WhatsThisCursor",
                                                                      u"WheelFocus");

    if (value.startsWith(u"Key_"))
        return true;

    return std::ranges::binary_search(list, QmlDesigner::ModelUtils::toStdStringView(value));
}

bool isKnownEnumScopes(QStringView value)
{
    static constexpr auto list = Utils::to_array<std::u16string_view>(
        u"TextInput",
        u"TextEdit",
        u"Material",
        u"Universal",
        u"Font",
        u"Shape",
        u"ShapePath",
        u"AbstractButton",
        u"Text",
        u"ShaderEffectSource",
        u"Grid",
        u"ItemLayer",
        u"ImageLayer",
        u"SpriteLayer",
        u"Light",
        u"ExtendedSceneEnvironment.GlowBlendMode");

    return std::ranges::find(list, QmlDesigner::ModelUtils::toStdStringView(value)) != std::end(list);
}

QString stripQuotes(const QString &str)
{
    if ((str.startsWith(QLatin1Char('"')) && str.endsWith(QLatin1Char('"')))
            || (str.startsWith(QLatin1Char('\'')) && str.endsWith(QLatin1Char('\''))))
        return str.mid(1, str.length() - 2);

    return str;
}

QString deEscape(const QString &value)
{
    QString result = value;

    result.replace("\\\\"_L1, "\\"_L1);
    result.replace("\\\""_L1, "\""_L1);
    result.replace("\\t"_L1, "\t"_L1);
    result.replace("\\r"_L1, "\r"_L1);
    result.replace("\\n"_L1, "\n"_L1);

    return result;
}

unsigned char convertHex(ushort c)
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

bool isHexDigit(ushort c)
{
    return ((c >= '0' && c <= '9')
            || (c >= 'a' && c <= 'f')
            || (c >= 'A' && c <= 'F'));
}


QString fixEscapedUnicodeChar(const QString &value) //convert "\u2939"
{
    if (value.size() == 6 && value.at(0) == QLatin1Char('\\') && value.at(1) == QLatin1Char('u')
        && isHexDigit(value.at(2).unicode()) && isHexDigit(value.at(3).unicode())
        && isHexDigit(value.at(4).unicode()) && isHexDigit(value.at(5).unicode())) {
        return convertUnicode(value.at(2).unicode(),
                              value.at(3).unicode(),
                              value.at(4).unicode(),
                              value.at(5).unicode());
    }
    return value;
}

bool isSignalPropertyName(QStringView signalName)
{
    if (signalName.isEmpty())
        return false;
    // see QmlCompiler::isSignalPropertyName
    auto [dummy, pureSignalName] = QmlDesigner::StringUtils::split_last(signalName, u'.');

    return pureSignalName.length() >= 3 && pureSignalName.startsWith(u"on")
           && pureSignalName.at(2).isLetter();
}

QVariant cleverConvert(const QString &value)
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

bool isLiteralValue(AST::ExpressionNode *expr)
{
    if (AST::cast<AST::NumericLiteral*>(expr))
        return true;
    if (AST::cast<AST::StringLiteral*>(expr))
        return true;
    else if (auto plusExpr = AST::cast<AST::UnaryPlusExpression*>(expr))
        return isLiteralValue(plusExpr->expression);
    else if (auto minusExpr = AST::cast<AST::UnaryMinusExpression*>(expr))
        return isLiteralValue(minusExpr->expression);
    else if (AST::cast<AST::TrueLiteral*>(expr))
        return true;
    else if (AST::cast<AST::FalseLiteral*>(expr))
        return true;
    else
        return false;
}

bool isLiteralValue(AST::Statement *stmt)
{
    auto exprStmt = AST::cast<AST::ExpressionStatement *>(stmt);
    if (exprStmt)
        return isLiteralValue(exprStmt->expression);
    else
        return false;
}

bool isLiteralValue(AST::UiScriptBinding *script)
{
    if (!script || !script->statement)
        return false;

    return isLiteralValue(script->statement);
}

int propertyType(const QString &typeName)
{
    if (typeName == u"bool")
        return QMetaType::fromName("bool").id();
    else if (typeName == u"color")
        return QMetaType::fromName("QColor").id();
    else if (typeName == u"date")
        return QMetaType::fromName("QDate").id();
    else if (typeName == u"int")
        return QMetaType::fromName("int").id();
    else if (typeName == u"real")
        return QMetaType::fromName("double").id();
    else if (typeName == u"double")
        return QMetaType::fromName("double").id();
    else if (typeName == u"string")
        return QMetaType::fromName("QString").id();
    else if (typeName == u"url")
        return QMetaType::fromName("QUrl").id();
    else if (typeName == u"var" || typeName == u"variant")
        return QMetaType::fromName("QVariant").id();
    else
        return -1;
}

QVariant convertDynamicPropertyValueToVariant(const QString &astValue,
                                                            const QString &astType)
{
    const QString cleanedValue = fixEscapedUnicodeChar(deEscape(stripQuotes(astValue.trimmed())));

    if (astType.isEmpty())
        return QVariant(QString());

    const QMetaType type = static_cast<QMetaType>(propertyType(astType));
    if (type == QMetaType::fromType<QVariant>()) {
        if (cleanedValue.isNull()) // Explicitly isNull, NOT isEmpty!
            return QVariant(type);
        else
            return QVariant(cleanedValue);
    } else {
        QVariant value = QVariant(cleanedValue);
        value.convert(type);
        return value;
    }
}

bool isListElementType(const QmlDesigner::TypeName &type)
{
    return type == "ListElement" || type == "QtQuick.ListElement" || type == "Qt.ListElement";
}

bool isPropertyChangesType(const QmlDesigner::TypeName &type)
{
    return type == "PropertyChanges" || type == "QtQuick.PropertyChanges" || type == "Qt.PropertyChanges";
}

bool isConnectionsType(const QmlDesigner::TypeName &type)
{
    return type == "Connections" || type == "QtQuick.Connections" || type == "Qt.Connections"
           || type == "QtQml.Connections" || type == "QtQml.Base.Connections";
}

bool propertyHasImplicitComponentType(const QmlDesigner::NodeAbstractProperty &property,
                                      const QmlDesigner::NodeMetaInfo &type)
{
    if (type.isQmlComponent())
        return false; //If the type is already a subclass of Component keep it

    return property.parentModelNode().isValid()
           && property.parentModelNode().metaInfo().property(property.name()).propertyType().isQmlComponent();
}

QString extractComponentFromQml(const QString &source)
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

QString normalizeJavaScriptExpression(const QString &expression)
{
    static const QRegularExpression regExp("\\n(\\s)+");

    QString result = expression;
    return result.replace(regExp, "\n");
}

bool compareJavaScriptExpression(const QString &expression1, const QString &expression2)
{
    return normalizeJavaScriptExpression(expression1) == normalizeJavaScriptExpression(expression2);
}

bool smartVeryFuzzyCompare(const QVariant &value1, const QVariant &value2)
{
    //we ignore slight changes on doubles and only check three digits
    const auto type1 = static_cast<QMetaType::Type>(value1.typeId());
    const auto type2 = static_cast<QMetaType::Type>(value2.typeId());
    if (type1 == QMetaType::Double
            || type2 == QMetaType::Double
            || type1 == QMetaType::Float
            || type2 == QMetaType::Float) {
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

    void removeModelNode(const QmlDesigner::ModelNode &modelNode)
    {
        QTC_ASSERT(modelNode.isValid(), return );
        modelNode.model()->removeModelNodes({modelNode},
                                            QmlDesigner::BypassModelResourceManagement::Yes);
    }
bool smartColorCompare(const QVariant &value1, const QVariant &value2)
{
    if ((value1.typeId() == QMetaType::QColor) || (value2.typeId() == QMetaType::QColor))
        return value1.value<QColor>().rgba() == value2.value<QColor>().rgba();
    return false;
}

    void removeProperty(const QmlDesigner::AbstractProperty &modelProperty)
    {
        QTC_ASSERT(modelProperty.isValid(), return );
        modelProperty.model()->removeProperties({modelProperty},
                                                QmlDesigner::BypassModelResourceManagement::Yes);
    }
bool equals(const QVariant &a, const QVariant &b)
{
    if (a.canConvert<QmlDesigner::Enumeration>() && b.canConvert<QmlDesigner::Enumeration>())
        return a.value<QmlDesigner::Enumeration>().toString() == b.value<QmlDesigner::Enumeration>().toString();
    if (a == b)
        return true;
    if (smartVeryFuzzyCompare(a, b))
        return true;
    if (smartColorCompare(a, b))
        return true;
    return false;
}

bool usesCustomParserButIsNotPropertyChange(const QmlDesigner::NodeMetaInfo &nodeMetaInfo)
{
    bool usesCustomParser = nodeMetaInfo.usesCustomParser();
    bool isQtQuickPropertyChanges = nodeMetaInfo.isQtQuickPropertyChanges();

    return usesCustomParser && !isQtQuickPropertyChanges;
}

} // anonymous namespace

namespace QmlDesigner {
using RewriterTracing::category;

namespace Internal {

class ReadingContext
{
public:
    ReadingContext([[maybe_unused]] const Snapshot &snapshot,
                   [[maybe_unused]] const Document::Ptr &doc,
                   [[maybe_unused]] const ViewerContext &vContext,
                   Model *model)
        : m_doc(doc)
        , m_model(model)
    {
    }

    ~ReadingContext() = default;

    Document::Ptr doc() const
    {
        return m_doc;
    }

    std::tuple<NodeMetaInfo, TypeName> lookup(AST::UiQualifiedId *astTypeNode)
    {
        TypeName fullTypeName;
        for (AST::UiQualifiedId *iter = astTypeNode; iter; iter = iter->next)
        if (!iter->name.isEmpty()) {
            fullTypeName += iter->name.toUtf8();
            if (iter->next)
                fullTypeName += '.';
        }

        NodeMetaInfo metaInfo = m_model->metaInfo(fullTypeName);
        return {metaInfo, fullTypeName};
    }

    bool lookupProperty(const QString &propertyPrefix,
                        const ModelNode &node,
                        const AST::UiQualifiedId *propertyId)
    {
        const QString propertyName = propertyPrefix.isEmpty() ? propertyId->name.toString()
                                                              : propertyPrefix;

        if (propertyName == u"id" && !propertyId->next)
            return false; // ### should probably be a special value

        //compare to lookupProperty(propertyPrefix, propertyId);
        return node.metaInfo().hasProperty(propertyName.toUtf8());
    }

    bool isArrayProperty(const AbstractProperty &property)
    {
        return ModelUtils::metainfo(property).isListProperty();
    }

    QVariant convertToVariant(const ModelNode &node,
                              const QString &astValue,
                              const QString &propertyPrefix,
                              AST::UiQualifiedId *propertyId)
    {
        const QString propertyName = propertyPrefix.isEmpty()
                                         ? toString(propertyId)
                                         : propertyPrefix + "." + toString(propertyId);


        const PropertyMetaInfo propertyMetaInfo = node.metaInfo().property(propertyName.toUtf8());
        const bool hasQuotes = astValue.trimmed().left(1) == u"\""
                               && astValue.trimmed().right(1) == u"\"";
        const QString cleanedValue = fixEscapedUnicodeChar(deEscape(stripQuotes(astValue.trimmed())));
        if (!propertyMetaInfo.isValid()) {
            const bool isAttached = !propertyName.isEmpty() && propertyName[0].isUpper();
            // Only list elements might have unknown properties.
            if (!node.metaInfo().isQtQmlModelsListElement() && !isAttached) {
                qCInfo(texttomodelMergerLog)
                    << Q_FUNC_INFO << "\nUnknown property"
                    << propertyPrefix + QLatin1Char('.') + toString(propertyId) << "on line"
                    << propertyId->identifierToken.startLine << "column"
                    << propertyId->identifierToken.startColumn;
            }
            return hasQuotes ? QVariant(cleanedValue) : cleverConvert(cleanedValue);
        }

        const NodeMetaInfo &propertyTypeMetaInfo = propertyMetaInfo.propertyType();

        if (propertyTypeMetaInfo.isColor())
            return PropertyParser::read(QMetaType::Type::QColor, cleanedValue);
        else if (propertyTypeMetaInfo.isUrl())
            return PropertyParser::read(QMetaType::Type::QUrl, cleanedValue);
        else if (propertyTypeMetaInfo.isVector2D())
            return PropertyParser::read(QMetaType::Type::QVector2D, cleanedValue);
        else if (propertyTypeMetaInfo.isVector3D())
            return PropertyParser::read(QMetaType::Type::QVector3D, cleanedValue);
        else if (propertyTypeMetaInfo.isVector4D())
            return PropertyParser::read(QMetaType::Type::QVector4D, cleanedValue);

        QVariant value(cleanedValue);
        if (propertyTypeMetaInfo.isBool()) {
            return value.toBool();
        } else if (propertyTypeMetaInfo.isInteger()) {
            return value.toInt();
        } else if (propertyTypeMetaInfo.isFloat()) {
            return value.toDouble();
        } else if (propertyTypeMetaInfo.isString()) {
            // nothing to do
        } else { //property alias et al
            if (!hasQuotes)
                return cleverConvert(cleanedValue);
        }
        return value;
    }

    QVariant convertToEnum(AST::Statement *rhs,
                           const NodeMetaInfo &metaInfo,
                           const QString &propertyPrefix,
                           AST::UiQualifiedId *propertyId,
                           QStringView astValue)
    {
        QList<QStringView> astValueList = astValue.split(u'.');

        if (astValueList.size() == 2) {
            //Check for global Qt enums
            if (astValueList.constFirst() == u"Qt" && isGlobalQtEnums(astValueList.constLast()))
                return QVariant::fromValue(Enumeration(astValue));

            //Check for known enum scopes used globally
            if (isKnownEnumScopes(astValueList.constFirst()))
                return QVariant::fromValue(Enumeration(astValue));
        } else if (astValueList.size() == 3) {
            QString enumName = astValueList.constFirst() + '.' + astValueList.at(1);
            if (isKnownEnumScopes(enumName))
                return QVariant::fromValue(
                    Enumeration(enumName.toUtf8(), astValueList.constLast().toUtf8()));
        }

        auto eStmt = AST::cast<AST::ExpressionStatement *>(rhs);
        if (!eStmt || !eStmt->expression)
            return QVariant();

        const QString propertyName = propertyPrefix.isEmpty() ? propertyId->name.toString()
                                                              : propertyPrefix;

        const PropertyMetaInfo pInfo = metaInfo.property(propertyName.toUtf8());

        if (pInfo.isEnumType())
            return QVariant::fromValue(Enumeration(astValue));
        else
            return QVariant();
    }

    QList<DiagnosticMessage> diagnosticLinkMessages() const
    { return m_diagnosticLinkMessages; }

private:
    Document::Ptr m_doc;
    QList<DiagnosticMessage> m_diagnosticLinkMessages;
    Model *m_model;
};

} // namespace Internal
} // namespace QmlDesigner

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

TextToModelMerger::TextToModelMerger(RewriterView *reWriterView) :
        m_rewriterView(reWriterView),
        m_isActive(false)
{
    Q_ASSERT(reWriterView);
    m_setupTimer.setSingleShot(true);
    RewriterView::connect(&m_setupTimer, &QTimer::timeout, reWriterView, &RewriterView::delayedSetup);
}

void TextToModelMerger::setActive(bool active)
{
    m_isActive = active;
}

bool TextToModelMerger::isActive() const
{
    return m_isActive;
}

void TextToModelMerger::setupImports(const Document::Ptr &doc, DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger setup imports", category()};

    if (!differenceHandler.isAmender())
        return;

    Imports existingImports = m_rewriterView->model()->imports();

    m_hasVersionlessImport = false;

    Imports imports;

    for (AST::UiHeaderItemList *iter = doc->qmlProgram()->headers; iter; iter = iter->next) {
        auto import = AST::cast<AST::UiImport *>(iter->headerItem);
        if (!import)
            continue;

        QString version;
        if (import->version != nullptr)
            version = QString("%1.%2").arg(import->version->majorVersion).arg(import->version->minorVersion);
        const QString &as = import->importId.toString();

        if (!import->fileName.isEmpty()) {
            const QString strippedFileName = stripQuotes(import->fileName.toString());
            imports.push_back(Import::createFileImport(strippedFileName,
                                                       version,
                                                       as,
                                                       m_rewriterView->importDirectories()));
        } else {
            QString importUri = toString(import->importUri);
            if (version.isEmpty())
                m_hasVersionlessImport = true;

            imports.push_back(Import::createLibraryImport(importUri,
                                                          version,
                                                          as,
                                                          m_rewriterView->importDirectories()));
        }
    }

    m_rewriterView->model()->setImports(imports);
}

Document::MutablePtr TextToModelMerger::createParsedDocument(const QUrl &url,
                                                             const QString &data,
                                                             QList<DocumentMessage> *errors)
{
    NanotraceHR::Tracer tracer{"text to model merger create parsed document", category()};

    if (data.isEmpty()) {
        if (errors) {
            QmlJS::DiagnosticMessage msg;
            msg.message = DesignerCore::Tr::tr("Empty document.");
            errors->append(DocumentMessage(msg, url));
        }
        return {};
    }

    Utils::FilePath fileName = Utils::FilePath::fromString(url.toLocalFile());

    Dialect dialect = ModelManagerInterface::guessLanguageOfFile(fileName);
    if (dialect == Dialect::AnyLanguage
            || dialect == Dialect::NoLanguage)
        dialect = Dialect::Qml;

    Document::MutablePtr doc = Document::create(fileName.isEmpty()
                                                    ? Utils::FilePath::fromString("<internal>")
                                                    : fileName,
                                                dialect);
    doc->setSource(data);
    doc->parseQml();

    if (!doc->isParsedCorrectly()) {
        if (errors) {
            const QList<QmlJS::DiagnosticMessage> messages = doc->diagnosticMessages();
            for (const QmlJS::DiagnosticMessage &message : messages)
                errors->append(DocumentMessage(message, doc->fileName().toUrl()));
        }
        return Document::MutablePtr();
    }
    return doc;
}

bool TextToModelMerger::load(const QString &data, DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger load", category()};

    QmlJS::ScopeChain::setSkipmakeComponentChain(true);
    const QScopeGuard cleanup([] { QmlJS::ScopeChain::setSkipmakeComponentChain(false); });

    qCInfo(rewriterBenchmark) << Q_FUNC_INFO;

    const bool justSanityCheck = !differenceHandler.isAmender();

    QElapsedTimer time;
    if (rewriterBenchmark().isInfoEnabled())
        time.start();

    const QUrl url = m_rewriterView->model()->fileUrl();

    m_qrcMapping.clear();
    addIsoIconQrcMapping(url);
    m_rewriterView->clearErrorAndWarnings();

    setActive(true);
    m_rewriterView->setIncompleteTypeInformation(false);

    // maybe the project environment (kit, ...) changed, so we need to clean old caches
    m_rewriterView->model()->clearMetaInfoCache();

    tracer.tick("text to model merger load - before try catch");
    try {
        Snapshot snapshot = TextModifier::qmljsSnapshot();

        QList<DocumentMessage> errors;
        QList<DocumentMessage> warnings;

        tracer.tick("text to model merger load - before create parsed document");
        if (Document::MutablePtr doc = createParsedDocument(url, data, &errors)) {
            /* We cannot do this since changes to other documents do have side effects on the current document
            if (m_document && (m_document->fingerprint() == doc->fingerprint())) {
                setActive(false);
                return true;
            }
            */

            snapshot.insert(doc);
            m_document = doc;
            qCInfo(rewriterBenchmark) << "parsed correctly: " << time.elapsed();
        } else {
            qCInfo(rewriterBenchmark) << "did not parse correctly: " << time.elapsed();
            m_rewriterView->setErrors(errors);
            setActive(false);
            return false;
        }


        m_vContext = ModelManagerInterface::instance()->projectVContext(Dialect::Qml, m_document);
        ReadingContext ctxt(snapshot, m_document, m_vContext, m_rewriterView->model());

        tracer.tick("text to model merger load - before setup imports");
        qCInfo(rewriterBenchmark) << "possible imports:" << time.elapsed();

        setupImports(m_document, differenceHandler);

        qCInfo(rewriterBenchmark) << "imports setup:" << time.elapsed();

        collectImportErrors(&errors);

        if (view()->checkSemanticErrors()) {
            collectSemanticErrorsAndWarningsAst(&errors, &warnings);

            if (!errors.isEmpty()) {
                m_rewriterView->setErrors(errors);
                setActive(false);
                clearPossibleImportKeys();
                return false;
            }
            if (!justSanityCheck)
                m_rewriterView->setWarnings(warnings);
            qCInfo(rewriterBenchmark) << "checked semantic errors:" << time.elapsed();
        }

        AST::UiObjectMember *astRootNode = nullptr;
        if (AST::UiProgram *program = m_document->qmlProgram())
            if (program->members)
                astRootNode = program->members->member;
        ModelNode modelRootNode = m_rewriterView->rootModelNode();

        tracer.tick("text to model merger load - before sync node");
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
    NanotraceHR::Tracer tracer{"text to model merger sync node -- recursive", category()};

    auto binding = AST::cast<AST::UiObjectBinding *>(astNode);

    const bool hasOnToken = binding && binding->hasOnToken;

    QString onTokenProperty;

    if (hasOnToken)
        onTokenProperty =  toString(binding->qualifiedId);

    AST::UiQualifiedId *astObjectType = qualifiedTypeNameId(astNode);
    AST::UiObjectInitializer *astInitializer = initializerOfObject(astNode);

    if (!astObjectType || !astInitializer)
        return;

    m_rewriterView->positionStorage()->setNodeOffset(modelNode, astObjectType->identifierToken.offset);

    auto [info, typeName] = context->lookup(astObjectType);
    if (!info.isValid()) {
        qWarning() << "Skipping node with unknown type" << toString(astObjectType);
        return;
    }

    if (modelNode.isRootNode() && !m_rewriterView->allowComponentRoot() && info.isQmlComponent()) {
        for (AST::UiObjectMemberList *iter = astInitializer->members; iter; iter = iter->next) {
            if (auto def = AST::cast<AST::UiObjectDefinition *>(iter->member)) {
                syncNode(modelNode, def, context, differenceHandler);
                return;
            }
        }
    }

    bool isImplicitComponent = modelNode.hasParentProperty()
                               && propertyHasImplicitComponentType(modelNode.parentProperty(), info);

    if (modelNode.metaInfo()
            != info //If there is no valid parentProperty                                                                                                      //the node has just been created. The type is correct then.
        || modelNode.behaviorPropertyName() != onTokenProperty) {
        const bool isRootNode = m_rewriterView->rootModelNode() == modelNode;
        differenceHandler.typeDiffers(isRootNode, modelNode, info, typeName, astNode, context);

        if (!modelNode.isValid())
            return;
    }

    if (info.isQmlComponent() || isImplicitComponent)
        setupComponentDelayed(modelNode, differenceHandler.isAmender());
    else if (usesCustomParserButIsNotPropertyChange(info))
        setupCustomParserNodeDelayed(modelNode, differenceHandler.isAmender());
    else if (!modelNode.nodeSource().isEmpty() || modelNode.nodeSourceType() != ModelNode::NodeWithoutSource)
        clearImplicitComponentDelayed(modelNode, differenceHandler.isAmender());

    QSet<PropertyName> modelPropertyNames = Utils::toSet(modelNode.propertyNames());
    if (!modelNode.id().isEmpty())
        modelPropertyNames.insert("id");
    QList<AST::UiObjectMember *> defaultPropertyItems;

    for (AST::UiObjectMemberList *iter = astInitializer->members; iter; iter = iter->next) {
        AST::UiObjectMember *member = iter->member;
        if (!member)
            continue;

        if (auto source = AST::cast<AST::UiSourceElement *>(member)) {
            auto function = AST::cast<AST::FunctionDeclaration *>(source->sourceElement);

            if (function) {
                AbstractProperty modelProperty = modelNode.property(function->name.toUtf8());

                QString astValue;
                if (function->body) {
                    astValue = textAt(context->doc(), function->lbraceToken, function->rbraceToken);
                    astValue = astValue.trimmed();
                }

                syncSignalHandler(modelProperty, astValue, differenceHandler);
                modelPropertyNames.remove(function->name.toUtf8());
            }
        } else if (auto array = AST::cast<AST::UiArrayBinding *>(member)) {
            const QString astPropertyName = toString(array->qualifiedId);
            if (isPropertyChangesType(typeName) || isConnectionsType(typeName)
                || modelNode.metaInfo().hasProperty(astPropertyName.toUtf8())) {
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
        } else if (auto def = AST::cast<AST::UiObjectDefinition *>(member)) {
            const QString &name = def->qualifiedTypeNameId->name.toString();
            if (name.isEmpty() || !name.at(0).isUpper()) {
                const QStringList props = syncGroupedProperties(modelNode,
                                                                name,
                                                                def->initializer->members,
                                                                context,
                                                                differenceHandler);
                for (const QString &prop : props)
                    modelPropertyNames.remove(prop.toUtf8());
            } else {
                defaultPropertyItems.append(member);
            }
        } else if (auto binding = AST::cast<AST::UiObjectBinding *>(member)) {
            const QString astPropertyName = toString(binding->qualifiedId);
            if (binding->hasOnToken) {
                // Store Behaviours in the default property
                defaultPropertyItems.append(member);
            } else {
                if (isPropertyChangesType(typeName) || isConnectionsType(typeName)
                    || modelNode.metaInfo().hasProperty(astPropertyName.toUtf8())) {
                    AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
                    if (context->isArrayProperty(modelProperty))
                        syncArrayProperty(modelProperty, {member}, context, differenceHandler);
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
        } else if (auto script = AST::cast<AST::UiScriptBinding *>(member)) {
            modelPropertyNames.remove(syncScriptBinding(modelNode, QString(), script, context, differenceHandler));
        } else if (auto property = AST::cast<AST::UiPublicMember *>(member)) {
            if (property->type == AST::UiPublicMember::Signal) {
                const QStringView astName = property->name;
                AbstractProperty modelProperty = modelNode.property(astName.toUtf8());
                QString signature = "()";
                if (property->parameters) {
                    signature = "("
                                + textAt(context->doc(),
                                         property->parameters->firstSourceLocation(),
                                         property->parameters->lastSourceLocation())
                                + ")";
                }

                syncSignalDeclarationProperty(modelProperty, signature, differenceHandler);
                modelPropertyNames.remove(astName.toUtf8());
                continue; // Done
            }

            const QStringView astName = property->name;
            QString astValue;
            if (property->statement)
                astValue = textAt(context->doc(),
                                  property->statement->firstSourceLocation(),
                                  property->statement->lastSourceLocation());

            astValue = astValue.trimmed();
            if (astValue.endsWith(QLatin1Char(';')))
                astValue = astValue.left(astValue.length() - 1);
            astValue = astValue.trimmed();

            const TypeName &astType = property->memberType->name.toUtf8();
            AbstractProperty modelProperty = modelNode.property(astName.toUtf8());

            if (property->binding) {
                if (auto binding = AST::cast<AST::UiObjectBinding *>(property->binding))
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
        } else if (AST::cast<AST::UiSourceElement *>(member)) {
            // function et al
        } else {
            qWarning() << "Found an unknown QML value.";
        }
    }

    PropertyName defaultPropertyName = info.defaultPropertyName();

    if (defaultPropertyName.isEmpty()) //fallback and use the meta system of the model
        defaultPropertyName = modelNode.metaInfo().defaultPropertyName();

    if (!defaultPropertyItems.isEmpty()) {
        if (modelNode.metaInfo().isQmlComponent())
            setupComponentDelayed(modelNode, differenceHandler.isAmender());
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

    for (const PropertyName &modelPropertyName : std::as_const(modelPropertyNames)) {
        AbstractProperty modelProperty = modelNode.property(modelPropertyName);

        // property deleted.
        if (modelPropertyName == "id")
            differenceHandler.idsDiffer(modelNode, QString());
        else
            differenceHandler.propertyAbsentFromQml(modelProperty);
    }
}

static QVariant parsePropertyExpression(AST::ExpressionNode *expressionNode)
{
    Q_ASSERT(expressionNode);

    NanotraceHR::Tracer tracer{"text to model merger parse property expression", category()};

    auto arrayLiteral = AST::cast<AST::ArrayPattern *>(expressionNode);

    if (arrayLiteral) {
        QList<QVariant> variantList;
        for (AST::PatternElementList *it = arrayLiteral->elements; it; it = it->next)
            variantList << parsePropertyExpression(it->element->initializer);
        return variantList;
    }

    auto stringLiteral = AST::cast<AST::StringLiteral *>(expressionNode);
    if (stringLiteral)
        return stringLiteral->value.toString();

    auto trueLiteral = AST::cast<AST::TrueLiteral *>(expressionNode);
    if (trueLiteral)
        return true;

    auto falseLiteral = AST::cast<AST::FalseLiteral *>(expressionNode);
    if (falseLiteral)
        return false;

    auto numericLiteral = AST::cast<AST::NumericLiteral *>(expressionNode);
    if (numericLiteral)
        return numericLiteral->value;


    return QVariant();
}

QVariant parsePropertyScriptBinding(AST::UiScriptBinding *uiScriptBinding)
{
    Q_ASSERT(uiScriptBinding);

    NanotraceHR::Tracer tracer{"text to model merger parse property script binding", category()};

    auto expStmt = AST::cast<AST::ExpressionStatement *>(uiScriptBinding->statement);
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
    NanotraceHR::Tracer tracer{"text to model merger sync script binding", category()};

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

    if (astPropertyName == u"id") {
        syncNodeId(modelNode, astValue, differenceHandler);
        return astPropertyName.toUtf8();
    }

    if (isSignalPropertyName(astPropertyName)) {
        AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
        syncSignalHandler(modelProperty, astValue, differenceHandler);
        return astPropertyName.toUtf8();
    }

    if (isLiteralValue(script)) {
        if (isPropertyChangesType(modelNode.type()) || isConnectionsType(modelNode.type())
            || isListElementType(modelNode.type())) {
            AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
            QVariant variantValue = parsePropertyScriptBinding(script);
            if (!variantValue.isValid())
                variantValue = deEscape(stripQuotes(astValue));
            syncVariantProperty(modelProperty, variantValue, TypeName(), differenceHandler);
            return astPropertyName.toUtf8();
        } else {
            const QVariant variantValue = context->convertToVariant(modelNode,
                                                                    astValue,
                                                                    prefix,
                                                                    script->qualifiedId);
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

    const QVariant enumValue = context->convertToEnum(script->statement,
                                                      modelNode.metaInfo(),
                                                      prefix,
                                                      script->qualifiedId,
                                                      astValue);

    const bool newlyCreatedTypeCase = false;

    if (enumValue.isValid()) { // It is a qualified enum:
        AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
        syncVariantProperty(modelProperty, enumValue, TypeName(), differenceHandler); // TODO: parse type
        return astPropertyName.toUtf8();
    } else { // Not an enum, so:
        if (isPropertyChangesType(modelNode.type()) || isConnectionsType(modelNode.type())
            || isSupportedAttachedProperties(astPropertyName)
            || modelNode.metaInfo().hasProperty(astPropertyName.toUtf8())
            || newlyCreatedTypeCase) {
            AbstractProperty modelProperty = modelNode.property(astPropertyName.toUtf8());
            syncExpressionProperty(modelProperty, astValue, TypeName(), differenceHandler); // TODO: parse type
            return astPropertyName.toUtf8();
        } else {
            qCInfo(texttomodelMergerLog) << Q_FUNC_INFO << "\nSkipping invalid expression property" << astPropertyName
                    << "for node type" << modelNode.type();
            return PropertyName();
        }
    }
}

void TextToModelMerger::syncNodeId(ModelNode &modelNode, const QString &astObjectId,
                                   DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger sync node id", category()};

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
    NanotraceHR::Tracer tracer{"text to model merger sync node property", category()};

    auto [info, typeName] = context->lookup(binding->qualifiedTypeNameId);

    if (!info.isValid()) {
        qWarning() << "SNP"
                   << "Skipping node with unknown type" << toString(binding->qualifiedTypeNameId);
        return;
    }

    if (modelProperty.isNodeProperty() && dynamicPropertyType == modelProperty.dynamicTypeName()) {
        ModelNode nodePropertyNode = modelProperty.toNodeProperty().modelNode();
        syncNode(nodePropertyNode, binding, context, differenceHandler);
    } else {
        differenceHandler.shouldBeNodeProperty(
            modelProperty, info, typeName, binding, dynamicPropertyType, context);
    }
}

void TextToModelMerger::syncExpressionProperty(AbstractProperty &modelProperty,
                                               const QString &javascript,
                                               const TypeName &astType,
                                               DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger sync expression property", category()};

    if (modelProperty.isBindingProperty()) {
        BindingProperty bindingProperty = modelProperty.toBindingProperty();
        if (!compareJavaScriptExpression(bindingProperty.expression(), javascript)
                || astType.isEmpty() == bindingProperty.isDynamic()
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
    NanotraceHR::Tracer tracer{"text to model merger sync signal handler", category()};

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
    NanotraceHR::Tracer tracer{"text to model merger sync array property", category()};

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
                                            const TypeName &astType,
                                            DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger sync variant property", category()};

    if (astValue.canConvert(QMetaType(QMetaType::QString)))
        populateQrcMapping(astValue.toString());

    if (modelProperty.isVariantProperty()) {
        VariantProperty modelVariantProperty = modelProperty.toVariantProperty();

        if (!equals(modelVariantProperty.value(), astValue)
                || astType.isEmpty() == modelVariantProperty.isDynamic()
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

void TextToModelMerger::syncSignalDeclarationProperty(AbstractProperty &modelProperty,
                                            const QString &signature,
                                            DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger sync signal declaration property", category()};

    if (modelProperty.isSignalDeclarationProperty()) {
        SignalDeclarationProperty signalHandlerProperty = modelProperty.toSignalDeclarationProperty();
        if (signalHandlerProperty.signature() != signature)
            differenceHandler.signalDeclarationSignatureDiffer(signalHandlerProperty, signature);
    } else {
        differenceHandler.shouldBeSignalDeclarationProperty(modelProperty, signature);
    }
}

void TextToModelMerger::syncNodeListProperty(NodeListProperty &modelListProperty,
                                             const QList<AST::UiObjectMember *> arrayMembers,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger sync node list property", category()};

    QList<AST::UiObjectMember *> fixedArrayMembers = Utils::filtered(
        arrayMembers, [context](const auto &arrayMember) {
            AST::UiQualifiedId *astObjectType = qualifiedTypeNameId(arrayMember);
            AST::UiObjectInitializer *astInitializer = initializerOfObject(arrayMember);
            if (astObjectType || astInitializer) {
                auto [info, typeName] = context->lookup(astObjectType);
                return info.isValid();
            }
        return false;
    });

    QList<ModelNode> modelNodes = modelListProperty.toModelNodeList();
    int i = 0;
    for (; i < modelNodes.size() && i < fixedArrayMembers.size(); ++i) {
        ModelNode modelNode = modelNodes.at(i);
        syncNode(modelNode, fixedArrayMembers.at(i), context, differenceHandler);
    }

    for (int j = i; j < fixedArrayMembers.size(); ++j) {
        // more elements in the dom-list, so add them to the model
        AST::UiObjectMember *arrayMember = fixedArrayMembers.at(j);
        const ModelNode newNode = differenceHandler.listPropertyMissingModelNode(modelListProperty, context, arrayMember);
    }

    for (int j = i; j < modelNodes.size(); ++j) {
        // more elements in the model, so remove them.
        ModelNode modelNode = modelNodes.at(j);
        differenceHandler.modelNodeAbsentFromQml(modelNode);
    }
}

ModelNode TextToModelMerger::createModelNode(const NodeMetaInfo &nodeMetaInfo,
                                             const TypeName &typeName,
                                             bool isImplicitComponent,
                                             AST::UiObjectMember *astNode,
                                             ReadingContext *context,
                                             DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger create model node", category()};

    QString nodeSource;

    auto binding = AST::cast<AST::UiObjectBinding *>(astNode);

    const bool hasOnToken = binding && binding->hasOnToken;

    QString onTokenProperty;
    if (hasOnToken)
        onTokenProperty =  toString(binding->qualifiedId);

    AST::UiQualifiedId *astObjectType = qualifiedTypeNameId(astNode);

    bool useCustomParser = usesCustomParserButIsNotPropertyChange(nodeMetaInfo);

    if (useCustomParser) {
        nodeSource = textAt(context->doc(),
                            astObjectType->identifierToken,
                            astNode->lastSourceLocation());
    }

    bool isQmlComponent = nodeMetaInfo.isQmlComponent();

    if (isQmlComponent || isImplicitComponent) {
        QString componentSource = extractComponentFromQml(textAt(context->doc(),
                                  astObjectType->identifierToken,
                                  astNode->lastSourceLocation()));


        nodeSource = componentSource;
    }

    ModelNode::NodeSourceType nodeSourceType = ModelNode::NodeWithoutSource;

    if (isQmlComponent || isImplicitComponent)
        nodeSourceType = ModelNode::NodeWithComponentSource;
    else if (useCustomParser)
        nodeSourceType = ModelNode::NodeWithCustomParserSource;

    ModelNode newNode = m_rewriterView->createModelNode(
        typeName, PropertyListType(), {}, nodeSource, nodeSourceType, onTokenProperty);

    syncNode(newNode, astNode, context, differenceHandler);
    return newNode;
}

QStringList TextToModelMerger::syncGroupedProperties(ModelNode &modelNode,
                                                     const QString &name,
                                                     AST::UiObjectMemberList *members,
                                                     ReadingContext *context,
                                                     DifferenceHandler &differenceHandler)
{
    NanotraceHR::Tracer tracer{"text to model merger sync grouped properties", category()};

    QStringList props;

    for (AST::UiObjectMemberList *iter = members; iter; iter = iter->next) {
        AST::UiObjectMember *member = iter->member;

        if (auto script = AST::cast<AST::UiScriptBinding *>(member)) {
            const QString prop = QString::fromLatin1(syncScriptBinding(modelNode, name, script, context, differenceHandler));
            if (!prop.isEmpty())
                props.append(prop);
        }
    }

    return props;
}

void ModelValidator::modelMissesImport([[maybe_unused]] const QmlDesigner::Import &import)
{
    Q_ASSERT(m_merger->view()->model()->imports().contains(import));
}

void ModelValidator::importAbsentInQMl([[maybe_unused]] const QmlDesigner::Import &import)
{
    QTC_ASSERT(!m_merger->view()->model()->imports().contains(import), return);
}

void ModelValidator::bindingExpressionsDiffer([[maybe_unused]] BindingProperty &modelProperty,
                                              [[maybe_unused]] const QString &javascript,
                                              [[maybe_unused]] const TypeName &astType)
{
    Q_ASSERT(compareJavaScriptExpression(modelProperty.expression(), javascript));
    Q_ASSERT(modelProperty.dynamicTypeName() == astType);
    Q_ASSERT(0);
}

void ModelValidator::shouldBeBindingProperty([[maybe_unused]] AbstractProperty &modelProperty,
                                             const QString & /*javascript*/,
                                             const TypeName & /*astType*/)
{
    Q_ASSERT(modelProperty.isBindingProperty());
    Q_ASSERT(0);
}

void ModelValidator::signalHandlerSourceDiffer([[maybe_unused]] SignalHandlerProperty &modelProperty,
                                               [[maybe_unused]] const QString &javascript)
{
    QTC_ASSERT(compareJavaScriptExpression(modelProperty.source(), javascript), return);
}

void ModelValidator::signalDeclarationSignatureDiffer(SignalDeclarationProperty &modelProperty,
                                                      const QString &signature)
{
    Q_UNUSED(modelProperty)
    Q_UNUSED(signature)
    QTC_ASSERT(compareJavaScriptExpression(modelProperty.signature(), signature), return);
}

void ModelValidator::shouldBeSignalHandlerProperty([[maybe_unused]] AbstractProperty &modelProperty,
                                                   const QString & /*javascript*/)
{
    Q_ASSERT(modelProperty.isSignalHandlerProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeSignalDeclarationProperty(AbstractProperty &modelProperty,
                                                       const QString & /*javascript*/)
{
    Q_UNUSED(modelProperty)
    Q_ASSERT(modelProperty.isSignalDeclarationProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeListProperty([[maybe_unused]] AbstractProperty &modelProperty,
                                              const QList<AST::UiObjectMember *> /*arrayMembers*/,
                                              ReadingContext * /*context*/)
{
    Q_ASSERT(modelProperty.isNodeListProperty());
    Q_ASSERT(0);
}

void ModelValidator::variantValuesDiffer([[maybe_unused]] VariantProperty &modelProperty,
                                         [[maybe_unused]] const QVariant &qmlVariantValue,
                                         [[maybe_unused]] const TypeName &dynamicTypeName)
{
    QTC_ASSERT(modelProperty.isDynamic() == !dynamicTypeName.isEmpty(), return);
    if (modelProperty.isDynamic()) {
        QTC_ASSERT(modelProperty.dynamicTypeName() == dynamicTypeName, return);
    }



    QTC_ASSERT(equals(modelProperty.value(), qmlVariantValue), qWarning() << modelProperty.value() << qmlVariantValue);
    QTC_ASSERT(0, return);
}

void ModelValidator::shouldBeVariantProperty([[maybe_unused]] AbstractProperty &modelProperty,
                                             const QVariant & /*qmlVariantValue*/,
                                             const TypeName & /*dynamicTypeName*/)
{
    QTC_CHECK(modelProperty.isVariantProperty());
    Q_ASSERT(modelProperty.isVariantProperty());
    Q_ASSERT(0);
}

void ModelValidator::shouldBeNodeProperty([[maybe_unused]] AbstractProperty &modelProperty,
                                          const NodeMetaInfo &,
                                          const TypeName & /*typeName*/,
                                          AST::UiObjectMember * /*astNode*/,
                                          const TypeName & /*dynamicPropertyType */,
                                          ReadingContext * /*context*/)
{
    Q_ASSERT(modelProperty.isNodeProperty());
    Q_ASSERT(0);
}

void ModelValidator::modelNodeAbsentFromQml([[maybe_unused]] ModelNode &modelNode)
{
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
                                 const NodeMetaInfo &,
                                 const TypeName &typeName,
                                 QmlJS::AST::UiObjectMember * /*astNode*/,
                                 ReadingContext * /*context*/)
{
    QTC_ASSERT(modelNode.type() == typeName, return);

    QTC_ASSERT(0, return);
}

void ModelValidator::propertyAbsentFromQml([[maybe_unused]] AbstractProperty &modelProperty)
{
    Q_ASSERT(!modelProperty.isValid());
    Q_ASSERT(0);
    QTC_CHECK(!modelProperty.isValid());
}

void ModelValidator::idsDiffer([[maybe_unused]] ModelNode &modelNode,
                               [[maybe_unused]] const QString &qmlId)
{
    QTC_ASSERT(modelNode.id() == qmlId, return);
    QTC_ASSERT(0, return);
}

void ModelAmender::modelMissesImport(const QmlDesigner::Import &) {}

void ModelAmender::importAbsentInQMl(const QmlDesigner::Import &) {}

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

void ModelAmender::signalDeclarationSignatureDiffer(SignalDeclarationProperty &modelProperty, const QString &signature)
{
    modelProperty.setSignature(signature);
}

void ModelAmender::shouldBeSignalHandlerProperty(AbstractProperty &modelProperty, const QString &javascript)
{
    ModelNode theNode = modelProperty.parentModelNode();
    SignalHandlerProperty newModelProperty = theNode.signalHandlerProperty(modelProperty.name());
    newModelProperty.setSource(javascript);
}

void ModelAmender::shouldBeSignalDeclarationProperty(AbstractProperty &modelProperty, const QString &signature)
{
    ModelNode theNode = modelProperty.parentModelNode();
    SignalDeclarationProperty newModelProperty = theNode.signalDeclarationProperty(modelProperty.name());
    newModelProperty.setSignature(signature);
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
                                        const NodeMetaInfo &nodeMetaInfo,
                                        const TypeName &typeName,
                                        AST::UiObjectMember *astNode,
                                        const TypeName &dynamicPropertyType,
                                        ReadingContext *context)
{
    ModelNode theNode = modelProperty.parentModelNode();
    NodeProperty newNodeProperty = theNode.nodeProperty(modelProperty.name());

    const bool propertyTakesComponent = propertyHasImplicitComponentType(newNodeProperty,
                                                                         nodeMetaInfo);

    const ModelNode &newNode = m_merger->createModelNode(
        nodeMetaInfo, typeName, propertyTakesComponent, astNode, context, *this);

    if (dynamicPropertyType.isEmpty())
        newNodeProperty.setModelNode(newNode);
    else
        newNodeProperty.setDynamicTypeNameAndsetModelNode(dynamicPropertyType, newNode);

    if (propertyTakesComponent)
        m_merger->setupComponentDelayed(newNode, true);

}

void ModelAmender::modelNodeAbsentFromQml(ModelNode &modelNode)
{
    removeModelNode(modelNode);
}

ModelNode ModelAmender::listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                                     ReadingContext *context,
                                                     AST::UiObjectMember *arrayMember)
{
    AST::UiQualifiedId *astObjectType = nullptr;
    AST::UiObjectInitializer *astInitializer = nullptr;
    if (auto def = AST::cast<AST::UiObjectDefinition *>(arrayMember)) {
        astObjectType = def->qualifiedTypeNameId;
        astInitializer = def->initializer;
    } else if (auto bin = AST::cast<AST::UiObjectBinding *>(arrayMember)) {
        astObjectType = bin->qualifiedTypeNameId;
        astInitializer = bin->initializer;
    }

    if (!astObjectType || !astInitializer)
        return ModelNode();

    auto [info, typeName] = context->lookup(astObjectType);
    if (!info.isValid()) {
        qWarning() << "Skipping node with unknown type" << toString(astObjectType);
        return {};
    }

    const bool propertyTakesComponent = propertyHasImplicitComponentType(modelProperty, info);

    const ModelNode &newNode = m_merger->createModelNode(
        info, typeName, propertyTakesComponent, arrayMember, context, *this);

    if (propertyTakesComponent)
        m_merger->setupComponentDelayed(newNode, true);

    if (modelProperty.isDefaultProperty()
        || modelProperty.parentModelNode().metaInfo().isQmlComponent()) { //In the default property case we do some magic
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
                               const NodeMetaInfo &nodeMetaInfo,
                               const TypeName &typeName,
                               AST::UiObjectMember *astNode,
                               ReadingContext *context)
{
    const bool propertyTakesComponent = modelNode.hasParentProperty()
                                        && propertyHasImplicitComponentType(modelNode.parentProperty(),
                                                                            nodeMetaInfo);

    if (isRootNode) {
        modelNode.view()->changeRootNodeType(typeName);
    } else {
        NodeAbstractProperty parentProperty = modelNode.parentProperty();
        int nodeIndex = -1;
        if (parentProperty.isNodeListProperty()) {
            nodeIndex = parentProperty.toNodeListProperty().indexOf(modelNode);
            Q_ASSERT(nodeIndex >= 0);
        }

        removeModelNode(modelNode);

        const ModelNode &newNode = m_merger->createModelNode(
            nodeMetaInfo, typeName, propertyTakesComponent, astNode, context, *this);
        parentProperty.reparentHere(newNode);
        if (parentProperty.isNodeListProperty()) {
            int currentIndex = parentProperty.toNodeListProperty().indexOf(newNode);
            if (nodeIndex != currentIndex)
                parentProperty.toNodeListProperty().slide(currentIndex, nodeIndex);
        }
    }
}

void ModelAmender::propertyAbsentFromQml(AbstractProperty &modelProperty)
{
    removeProperty(modelProperty);
}

void ModelAmender::idsDiffer(ModelNode &modelNode, const QString &qmlId)
{
    modelNode.setIdWithoutRefactoring(qmlId);
}

void TextToModelMerger::setupComponent(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"text to model merger setup component", category()};

    if (!node.isValid())
        return;

    QString componentText = m_rewriterView->extractText({node}).value(node);

    if (componentText.isEmpty() && node.nodeSource().isEmpty())
        return;

    QString result = extractComponentFromQml(componentText);

    if (result.isEmpty() && node.nodeSource().isEmpty())
        return; //No object definition found

    if (node.nodeSource() != result)
        ModelNode(node).setNodeSource(result, ModelNode::NodeWithComponentSource);
}

void TextToModelMerger::clearImplicitComponent(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"text to model merger clear implicit component", category()};

    ModelNode(node).setNodeSource({}, ModelNode::NodeWithoutSource);
}

void TextToModelMerger::collectLinkErrors(QList<DocumentMessage> *errors, const ReadingContext &ctxt)
{
    NanotraceHR::Tracer tracer{"text to model merger collect link errors", category()};

    const QList<QmlJS::DiagnosticMessage> diagnosticMessages = ctxt.diagnosticLinkMessages();
    for (const QmlJS::DiagnosticMessage &diagnosticMessage : diagnosticMessages) {
        if (diagnosticMessage.kind == QmlJS::Severity::ReadingTypeInfoWarning)
            m_rewriterView->setIncompleteTypeInformation(true);

        errors->append(
            DocumentMessage(diagnosticMessage, QUrl::fromLocalFile(m_document->fileName().path())));
    }
}

void TextToModelMerger::collectImportErrors(QList<DocumentMessage> *errors)
{
    NanotraceHR::Tracer tracer{"text to model merger collect import errors", category()};

    if (m_rewriterView->model()->imports().isEmpty()) {
        const QmlJS::DiagnosticMessage diagnosticMessage(QmlJS::Severity::Error,
                                                         SourceLocation(0, 0, 0, 0),
                                                         DesignerCore::Tr::tr(
                                                             "No import statements found."));
        errors->append(
            DocumentMessage(diagnosticMessage, QUrl::fromLocalFile(m_document->fileName().path())));
    }

    bool hasQtQuick = false;
    for (const QmlDesigner::Import &import : m_rewriterView->model()->imports()) {
        if (import.isLibraryImport() && import.url() == u"QtQuick") {
            hasQtQuick = true;

            auto &externalDependencies = m_rewriterView->externalDependencies();
            if (externalDependencies.hasStartupTarget()) {
                const bool qt6import = !import.hasVersion() || import.majorVersion() == 6;

                if (!externalDependencies.isQt6Import() && (m_hasVersionlessImport || qt6import)) {
                    const QmlJS::DiagnosticMessage diagnosticMessage(
                        QmlJS::Severity::Error,
                        SourceLocation(0, 0, 0, 0),
                        DesignerCore::Tr::tr("Qt Quick 6 is not supported with a Qt 5 kit."));
                    errors->prepend(
                        DocumentMessage(diagnosticMessage,
                                        QUrl::fromLocalFile(m_document->fileName().path())));
                }
            } else {
                const QmlJS::DiagnosticMessage diagnosticMessage(
                    QmlJS::Severity::Error,
                    SourceLocation(0, 0, 0, 0),
                    DesignerCore::Tr::tr("The Design Mode requires a valid Qt kit."));
                errors->prepend(DocumentMessage(diagnosticMessage,
                                                QUrl::fromLocalFile(m_document->fileName().path())));
            }
        }
    }

    if (!hasQtQuick)
        errors->append(DocumentMessage(DesignerCore::Tr::tr("No import for Qt Quick found.")));
}

void TextToModelMerger::collectSemanticErrorsAndWarningsAst(
    [[maybe_unused]] QList<DocumentMessage> *errors, [[maybe_unused]] QList<DocumentMessage> *warnings)
{
    NanotraceHR::Tracer tracer{"text to model merger collect semantic errors and warnings",
                               category()};

    QList<StaticAnalysis::Message> messages;

    AstCheck check(m_document);
    messages = check();

    QUrl fileNameUrl = QUrl::fromLocalFile(m_document->fileName().toUrlishString());

    for (const StaticAnalysis::Message &message : messages) {
        if (message.severity == Severity::Error) {
            if (message.type == StaticAnalysis::ErrUnknownComponent)
                warnings->append(DocumentMessage(message.toDiagnosticMessage(), fileNameUrl));
            else
                errors->append(DocumentMessage(message.toDiagnosticMessage(), fileNameUrl));
        }
        if (message.severity == Severity::Warning)
            warnings->append(DocumentMessage(message.toDiagnosticMessage(), fileNameUrl));
    }
}

void TextToModelMerger::populateQrcMapping(const QString &filePath)
{
    NanotraceHR::Tracer tracer{"text to model merger populate QRC mapping", category()};

    if (!filePath.startsWith(QLatin1String("qrc:")))
        return;

    auto [pathView, fileNameView] = StringUtils::split_last(filePath, u'/');

    QString path = pathView.toString();
    const QString fileName = fileNameView.toString();
    path.remove(QLatin1String("qrc:"));
    QMap<QString,QStringList> map = ModelManagerInterface::instance()->filesInQrcPath(path);
    const QStringList qrcFilePaths = map.value(fileName, {});
    if (!qrcFilePaths.isEmpty()) {
        QString fileSystemPath = qrcFilePaths.constFirst();
        fileSystemPath.remove(fileName);
        if (path.isEmpty())
            path.prepend(QLatin1String("/"));
        m_qrcMapping.insert({path, fileSystemPath});
    }
}

void TextToModelMerger::addIsoIconQrcMapping(const QUrl &fileUrl)
{
    NanotraceHR::Tracer tracer{"text to model merger add ISO icon QRC mapping", category()};

    QDir dir(fileUrl.toLocalFile());
    do {
        if (!dir.entryList({"*.pro"}, QDir::Files).isEmpty()) {
            m_qrcMapping.insert({"/iso-icons", dir.absolutePath() + "/iso-icons"});
            return;
        }
    } while (dir.cdUp());
}

void TextToModelMerger::setupComponentDelayed(const ModelNode &node, bool synchronous)
{
    NanotraceHR::Tracer tracer{"text to model merger setup component delayed", category()};

    if (synchronous) {
        setupComponent(node);
    } else {
        m_setupComponentList.insert(node);
        m_setupTimer.start();
    }
}

void TextToModelMerger::setupCustomParserNode(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"text to model merger setup custom parser node", category()};

    if (!node.isValid())
        return;

    QString modelText = m_rewriterView->extractText({node}).value(node);

    if (modelText.isEmpty() && node.nodeSource().isEmpty())
        return;

    if (node.nodeSource() != modelText)
        ModelNode(node).setNodeSource(modelText, ModelNode::NodeWithCustomParserSource);

}

void TextToModelMerger::setupCustomParserNodeDelayed(const ModelNode &node, bool synchronous)
{
    NanotraceHR::Tracer tracer{"text to model merger setup custom parser node delayed", category()};

    Q_ASSERT(usesCustomParserButIsNotPropertyChange(node.metaInfo()));

    if (synchronous) {
        setupCustomParserNode(node);
    } else {
        m_setupCustomParserList.insert(node);
        m_setupTimer.start();
    }
}

void TextToModelMerger::clearImplicitComponentDelayed(const ModelNode &node, bool synchronous)
{
    Q_ASSERT(!usesCustomParserButIsNotPropertyChange(node.metaInfo()));

    if (synchronous) {
        clearImplicitComponent(node);
    } else {
        m_clearImplicitComponentList.insert(node);
        m_setupTimer.start();
    }
}

void TextToModelMerger::delayedSetup()
{
    NanotraceHR::Tracer tracer{"text to model merger delayed setup", category()};

    for (const ModelNode &node : std::as_const(m_setupComponentList))
        setupComponent(node);

    for (const ModelNode &node : std::as_const(m_setupCustomParserList))
        setupCustomParserNode(node);

    for (const ModelNode &node : std::as_const(m_clearImplicitComponentList))
        clearImplicitComponent(node);

    m_setupCustomParserList.clear();
    m_setupComponentList.clear();
    m_clearImplicitComponentList.clear();
}

QSet<QPair<QString, QString> > TextToModelMerger::qrcMapping() const
{
    return m_qrcMapping;
}

QList<QmlTypeData> TextToModelMerger::getQMLSingletons() const
{
    return {};
}

void TextToModelMerger::clearPossibleImportKeys()
{
    m_possibleModules.clear();
    m_previousPossibleModulesSize = -1;
}

void TextToModelMerger::setRemoveImports(bool removeImports)
{
    m_removeImports = removeImports;
}

QString TextToModelMerger::textAt(const Document::Ptr &doc,
                                  const SourceLocation &location)
{
    NanotraceHR::Tracer tracer{"text to model merger text at", category()};
    return doc->source().mid(location.offset, location.length);
}

QString TextToModelMerger::textAt(const Document::Ptr &doc,
                                  const SourceLocation &from,
                                  const SourceLocation &to)
{
    NanotraceHR::Tracer tracer{"text to model merger text at from to", category()};
    return doc->source().mid(from.offset, to.end() - from.begin());
}
