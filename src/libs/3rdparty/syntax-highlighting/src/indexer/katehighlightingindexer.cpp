/*
    SPDX-FileCopyrightText: 2014 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include <QBuffer>
#include <QCborValue>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutableMapIterator>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QString>
#include <QVariant>
#include <QXmlStreamReader>

#ifdef HAS_XERCESC

#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/XMLGrammarPoolImpl.hpp>

#include <xercesc/parsers/SAX2XMLReaderImpl.hpp>

#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUni.hpp>

#include <xercesc/framework/XMLGrammarPoolImpl.hpp>
#include <xercesc/validators/common/Grammar.hpp>

using namespace xercesc;

/*
 * Ideas taken from:
 *
 * author    : Boris Kolpackov <boris@codesynthesis.com>
 * copyright : not copyrighted - public domain
 *
 * This program uses Xerces-C++ SAX2 parser to load a set of schema files
 * and then to validate a set of XML documents against these schemas. To
 * build this program you will need Xerces-C++ 3.0.0 or later. For more
 * information, see:
 *
 * http://www.codesynthesis.com/~boris/blog/2010/03/15/validating-external-schemas-xerces-cxx/
 */

/**
 * Error handler object used during xml schema validation.
 */
class CustomErrorHandler : public ErrorHandler
{
public:
    /**
     * Constructor
     * @param messages Pointer to the error message string to fill.
     */
    CustomErrorHandler(QString *messages)
        : m_messages(messages)
    {
    }

    /**
     * Check global success/fail state.
     * @return True if there was a failure, false otherwise.
     */
    bool failed() const
    {
        return m_failed;
    }

private:
    /**
     * Severity classes for error messages.
     */
    enum severity { s_warning, s_error, s_fatal };

    /**
     * Wrapper for warning exceptions.
     * @param e Exception to handle.
     */
    void warning(const SAXParseException &e) override
    {
        m_failed = true; // be strict, warnings are evil, too!
        handle(e, s_warning);
    }

    /**
     * Wrapper for error exceptions.
     * @param e Exception to handle.
     */
    void error(const SAXParseException &e) override
    {
        m_failed = true;
        handle(e, s_error);
    }

    /**
     * Wrapper for fatal error exceptions.
     * @param e Exception to handle.
     */
    void fatalError(const SAXParseException &e) override
    {
        m_failed = true;
        handle(e, s_fatal);
    }

    /**
     * Reset the error status to "no error".
     */
    void resetErrors() override
    {
        m_failed = false;
    }

    /**
     * Generic handler for error/warning/fatal error message exceptions.
     * @param e Exception to handle.
     * @param s Enum value encoding the message severtity.
     */
    void handle(const SAXParseException &e, severity s)
    {
        // get id to print
        const XMLCh *xid(e.getPublicId());
        if (!xid)
            xid = e.getSystemId();

        m_messages << QString::fromUtf16(xid) << ":" << e.getLineNumber() << ":" << e.getColumnNumber() << " " << (s == s_warning ? "warning: " : "error: ")
                   << QString::fromUtf16(e.getMessage()) << Qt::endl;
    }

private:
    /**
     * Storage for created error messages in this handler.
     */
    QTextStream m_messages;

    /**
     * Global error state. True if there was an error, false otherwise.
     */
    bool m_failed = false;
};

class CustomXMLValidator : public SAX2XMLReaderImpl
{
public:
    QString messages;
    CustomErrorHandler eh{&messages};

    CustomXMLValidator(XMLGrammarPool *xsd)
        : SAX2XMLReaderImpl(XMLPlatformUtils::fgMemoryManager, xsd)
    {
        // Commonly useful configuration.
        //
        setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
        setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, true);
        setFeature(XMLUni::fgSAX2CoreValidation, true);

        // Enable validation.
        //
        setFeature(XMLUni::fgXercesSchema, true);
        setFeature(XMLUni::fgXercesSchemaFullChecking, true);
        setFeature(XMLUni::fgXercesValidationErrorAsFatal, true);

        // Use the loaded grammar during parsing.
        //
        setFeature(XMLUni::fgXercesUseCachedGrammarInParse, true);

        // Don't load schemas from any other source (e.g., from XML document's
        // xsi:schemaLocation attributes).
        //
        setFeature(XMLUni::fgXercesLoadSchema, false);

        // Xerces-C++ 3.1.0 is the first version with working multi import
        // support.
        //
        setFeature(XMLUni::fgXercesHandleMultipleImports, true);

        setErrorHandler(&eh);
    }
};

#endif

#include "../lib/worddelimiters_p.h"
#include "../lib/xml_p.h"

#include <array>

using KSyntaxHighlighting::WordDelimiters;
using KSyntaxHighlighting::Xml::attrToBool;

using namespace Qt::Literals::StringLiterals;

static constexpr QStringView operator""_sv(const char16_t *s, std::size_t n)
{
    return QStringView(s, s + n);
}

namespace
{

struct KateVersion {
    int majorRevision;
    int minorRevision;

    KateVersion(int majorRevision = 0, int minorRevision = 0)
        : majorRevision(majorRevision)
        , minorRevision(minorRevision)
    {
    }

    bool operator<(const KateVersion &version) const
    {
        return majorRevision < version.majorRevision || (majorRevision == version.majorRevision && minorRevision < version.minorRevision);
    }
};

class HlFilesChecker
{
public:
    void setDefinition(QStringView verStr, const QString &filename, const QString &name, const QStringList &alternativeNames)
    {
        m_currentDefinition = &*m_definitions.insert(name, Definition{});
        m_currentDefinition->languageName = name;
        m_currentDefinition->filename = filename;
        m_currentDefinition->kateVersionStr = verStr.toString();
        m_currentKeywords = nullptr;
        m_currentContext = nullptr;

        const auto idx = verStr.indexOf(u'.');
        if (idx <= 0) {
            qWarning() << filename << "invalid kateversion" << verStr;
            m_success = false;
        } else {
            m_currentDefinition->kateVersion = {verStr.sliced(0, idx).toInt(), verStr.sliced(idx + 1).toInt()};
        }

        auto checkName = [this, &filename](char const *nameType, const QString &name) {
            auto it = m_names.find(name);
            if (it != m_names.end()) {
                qWarning() << filename << "duplicate" << nameType << "with" << it.value();
                m_success = false;
            } else {
                m_names.insert(name, filename);
            }
        };
        checkName("name", name);
        for (const auto &alternativeName : alternativeNames) {
            checkName("alternative name", alternativeName);
        }
    }

    KateVersion currentVersion() const
    {
        return m_currentDefinition->kateVersion;
    }

    void processElement(const QXmlStreamReader &xml)
    {
        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (m_currentContext) {
                m_currentContext->rules.push_back(Context::Rule{});
                auto &rule = m_currentContext->rules.back();
                m_success = rule.parseElement(m_currentDefinition->filename, xml) && m_success;
                m_currentContext->hasDynamicRule = m_currentContext->hasDynamicRule || rule.dynamic == XmlBool::True;
            } else if (m_currentKeywords) {
                m_inKeywordItem = true;
            } else if (xml.name() == u"context"_sv) {
                processContextElement(xml);
            } else if (xml.name() == u"list"_sv) {
                processListElement(xml);
            } else if (xml.name() == u"keywords"_sv) {
                m_success = m_currentDefinition->parseKeywords(xml) && m_success;
            } else if (xml.name() == u"emptyLine"_sv) {
                m_success = parseEmptyLine(m_currentDefinition->filename, xml) && m_success;
            } else if (xml.name() == u"itemData"_sv) {
                m_success = m_currentDefinition->itemDatas.parseElement(m_currentDefinition->filename, xml) && m_success;
            }
            break;

        case QXmlStreamReader::EndElement:
            if (m_currentContext && xml.name() == u"context"_sv) {
                m_currentContext = nullptr;
            } else if (m_currentKeywords && xml.name() == u"list"_sv) {
                m_currentKeywords = nullptr;
            } else if (m_currentKeywords) {
                m_success = m_currentKeywords->items.parseElement(m_currentDefinition->filename, xml, m_textContent) && m_success;
                m_textContent.clear();
                m_inKeywordItem = false;
            }
            break;

        case QXmlStreamReader::EntityReference:
        case QXmlStreamReader::Characters:
            if (m_inKeywordItem) {
                m_textContent += xml.text();
            }
            break;

        default:;
        }
    }

    //! Resolve context attribute and include tag
    void resolveContexts()
    {
        QMutableMapIterator<QString, Definition> def(m_definitions);
        while (def.hasNext()) {
            def.next();
            auto &definition = def.value();
            auto &contexts = definition.contexts;

            if (contexts.isEmpty()) {
                qWarning() << definition.filename << "has no context";
                m_success = false;
                continue;
            }

            auto markAsUsedContext = [](ContextName &contextName) {
                if (!contextName.stay && contextName.context) {
                    contextName.context->isOnlyIncluded = false;
                }
            };

            QMutableMapIterator<QString, Context> contextIt(contexts);
            while (contextIt.hasNext()) {
                contextIt.next();
                auto &context = contextIt.value();
                resolveContextName(definition, context, context.lineEndContext, context.line);
                resolveContextName(definition, context, context.lineEmptyContext, context.line);
                resolveContextName(definition, context, context.fallthroughContext, context.line);
                markAsUsedContext(context.lineEndContext);
                markAsUsedContext(context.lineEmptyContext);
                markAsUsedContext(context.fallthroughContext);
                for (auto &rule : context.rules) {
                    rule.parentContext = &context;
                    resolveContextName(definition, context, rule.context, rule.line);
                    if (rule.type != Context::Rule::Type::IncludeRules) {
                        markAsUsedContext(rule.context);
                    } else if (rule.includeAttrib == XmlBool::True && rule.context.context) {
                        rule.context.context->referencedWithIncludeAttrib = true;
                    }
                }
            }

            auto *firstContext = &*definition.contexts.find(definition.firstContextName);
            firstContext->isOnlyIncluded = false;
            definition.firstContext = firstContext;
        }

        resolveIncludeRules();
    }

    bool check() const
    {
        bool success = m_success;

        const auto usedContexts = extractUsedContexts();

        QMap<const Definition *, const Definition *> maxVersionByDefinitions;
        QMap<const Context::Rule *, IncludedRuleUnreachableBy> unreachableIncludedRules;

        QMapIterator<QString, Definition> def(m_definitions);
        while (def.hasNext()) {
            def.next();
            const auto &definition = def.value();
            const auto &filename = definition.filename;

            auto *maxDef = maxKateVersionDefinition(definition, maxVersionByDefinitions);
            if (maxDef != &definition) {
                qWarning() << definition.filename << "depends on a language" << maxDef->languageName << "in version" << maxDef->kateVersionStr
                           << ". Please, increase kateversion.";
                success = false;
            }

            QSet<ItemDatas::Style> usedAttributeNames;
            QSet<ItemDatas::Style> ignoredAttributeNames;
            success = checkKeywordsList(definition) && success;
            success = checkContexts(definition, usedAttributeNames, ignoredAttributeNames, usedContexts, unreachableIncludedRules) && success;

            // search for non-existing itemDatas.
            const auto invalidNames = usedAttributeNames - definition.itemDatas.styleNames;
            for (const auto &styleName : invalidNames) {
                qWarning() << filename << "line" << styleName.line << "reference of non-existing itemData attributes:" << styleName.name;
                success = false;
            }

            // search for existing itemDatas, but unusable.
            const auto ignoredNames = ignoredAttributeNames - usedAttributeNames;
            for (const auto &styleName : ignoredNames) {
                qWarning() << filename << "line" << styleName.line << "attribute" << styleName.name
                           << "is never used. All uses are with lookAhead=true or <IncludeRules/>";
                success = false;
            }

            // search for unused itemDatas.
            auto unusedNames = definition.itemDatas.styleNames - usedAttributeNames;
            unusedNames -= ignoredNames;
            for (const auto &styleName : std::as_const(unusedNames)) {
                qWarning() << filename << "line" << styleName.line << "unused itemData:" << styleName.name;
                success = false;
            }
        }

        QMutableMapIterator<const Context::Rule *, IncludedRuleUnreachableBy> unreachableIncludedRuleIt(unreachableIncludedRules);
        while (unreachableIncludedRuleIt.hasNext()) {
            unreachableIncludedRuleIt.next();
            IncludedRuleUnreachableBy &unreachableRulesBy = unreachableIncludedRuleIt.value();
            if (unreachableRulesBy.alwaysUnreachable) {
                auto *rule = unreachableIncludedRuleIt.key();

                if (!rule->parentContext->isOnlyIncluded) {
                    continue;
                }

                // remove duplicates rules
                QSet<const Context::Rule *> rules;
                auto &unreachableBy = unreachableRulesBy.unreachableBy;
                unreachableBy.erase(std::remove_if(unreachableBy.begin(),
                                                   unreachableBy.end(),
                                                   [&](const RuleAndInclude &ruleAndInclude) {
                                                       if (rules.contains(ruleAndInclude.rule)) {
                                                           return true;
                                                       }
                                                       rules.insert(ruleAndInclude.rule);
                                                       return false;
                                                   }),
                                    unreachableBy.end());

                QString message;
                message.reserve(128);
                for (auto &ruleAndInclude : std::as_const(unreachableBy)) {
                    message += u"line "_sv;
                    message += QString::number(ruleAndInclude.rule->line);
                    message += u" ["_sv;
                    message += ruleAndInclude.rule->parentContext->name;
                    if (rule->filename != ruleAndInclude.rule->filename) {
                        message += u" ("_sv;
                        message += ruleAndInclude.rule->filename;
                        message += u')';
                    }
                    if (ruleAndInclude.includeRules) {
                        message += u" via line "_sv;
                        message += QString::number(ruleAndInclude.includeRules->line);
                    }
                    message += u"], "_sv;
                }
                message.chop(2);

                qWarning() << rule->filename << "line" << rule->line << "no IncludeRule can reach this rule, hidden by" << message;
                success = false;
            }
        }

        return success;
    }

private:
    enum class XmlBool {
        Unspecified,
        False,
        True,
    };

    struct Context;

    struct ContextName {
        QString name;
        int popCount = 0;
        bool stay = false;

        Context *context = nullptr;
    };

    struct Parser {
        const QString &filename;
        const QXmlStreamReader &xml;
        const QXmlStreamAttribute &attr;
        bool success;

        //! Read a string type attribute, \c success = \c false when \p str is not empty
        //! \return \c true when attr.name() == attrName, otherwise false
        bool extractString(QString &str, QStringView attrName)
        {
            if (attr.name() != attrName) {
                return false;
            }

            str = attr.value().toString();
            if (str.isEmpty()) {
                qWarning() << filename << "line" << xml.lineNumber() << attrName << "attribute is empty";
                success = false;
            }

            return true;
        }

        //! Read a bool type attribute, \c success = \c false when \p xmlBool is not \c XmlBool::Unspecified.
        //! \return \c true when attr.name() == attrName, otherwise false
        bool extractXmlBool(XmlBool &xmlBool, QStringView attrName)
        {
            if (attr.name() != attrName) {
                return false;
            }

            xmlBool = attr.value().isNull() ? XmlBool::Unspecified : attrToBool(attr.value()) ? XmlBool::True : XmlBool::False;

            return true;
        }

        //! Read a positive integer type attribute, \c success = \c false when \p positive is already greater than or equal to 0
        //! \return \c true when attr.name() == attrName, otherwise false
        bool extractPositive(int &positive, QStringView attrName)
        {
            if (attr.name() != attrName) {
                return false;
            }

            bool ok = true;
            positive = attr.value().toInt(&ok);

            if (!ok || positive < 0) {
                qWarning() << filename << "line" << xml.lineNumber() << attrName << "should be a positive integer:" << attr.value();
                success = false;
            }

            return true;
        }

        //! Read a color, \c success = \c false when \p color is already greater than or equal to 0
        //! \return \c true when attr.name() == attrName, otherwise false
        bool checkColor(QStringView attrName)
        {
            if (attr.name() != attrName) {
                return false;
            }

            const auto value = attr.value();
            if (value.isEmpty() /*|| QColor(value).isValid()*/) {
                qWarning() << filename << "line" << xml.lineNumber() << attrName << "should be a color:" << value;
                success = false;
            }

            return true;
        }

        //! Read a QChar, \c success = \c false when \p c is not \c '\0' or does not have one char
        //! \return \c true when attr.name() == attrName, otherwise false
        bool extractChar(QChar &c, QStringView attrName)
        {
            if (attr.name() != attrName) {
                return false;
            }

            if (attr.value().size() == 1) {
                c = attr.value()[0];
            } else {
                c = u'_';
                qWarning() << filename << "line" << xml.lineNumber() << attrName << "must contain exactly one char:" << attr.value();
                success = false;
            }

            return true;
        }

        //! \return parsing status when \p isExtracted is \c true, otherwise \c false
        bool checkIfExtracted(bool isExtracted)
        {
            if (isExtracted) {
                return success;
            }

            qWarning() << filename << "line" << xml.lineNumber() << "unknown attribute:" << attr.name();
            return false;
        }
    };

    struct Keywords {
        struct Items {
            struct Item {
                QString content;
                int line;

                friend size_t qHash(const Item &item, size_t seed = 0)
                {
                    return qHash(item.content, seed);
                }

                friend bool operator==(const Item &item0, const Item &item1)
                {
                    return item0.content == item1.content;
                }
            };

            QList<Item> keywords;
            QSet<Item> includes;

            bool parseElement(const QString &filename, const QXmlStreamReader &xml, const QString &content)
            {
                bool success = true;

                const int line = xml.lineNumber();

                if (content.isEmpty()) {
                    qWarning() << filename << "line" << line << "is empty:" << xml.name();
                    success = false;
                }

                if (xml.name() == u"include"_sv) {
                    includes.insert({content, line});
                } else if (xml.name() == u"item"_sv) {
                    keywords.append({content, line});
                } else {
                    qWarning() << filename << "line" << line << "invalid element:" << xml.name();
                    success = false;
                }

                return success;
            }
        };

        QString name;
        Items items;
        int line;

        bool parseElement(const QString &filename, const QXmlStreamReader &xml)
        {
            line = xml.lineNumber();

            bool success = true;
            const auto attrs = xml.attributes();
            for (const auto &attr : attrs) {
                Parser parser{filename, xml, attr, success};

                const bool isExtracted = parser.extractString(name, u"name"_sv);

                success = parser.checkIfExtracted(isExtracted);
            }
            return success;
        }
    };

    struct Context {
        struct Rule {
            enum class Type {
                Unknown,
                AnyChar,
                Detect2Chars,
                DetectChar,
                DetectIdentifier,
                DetectSpaces,
                Float,
                HlCChar,
                HlCHex,
                HlCOct,
                HlCStringChar,
                IncludeRules,
                Int,
                LineContinue,
                RangeDetect,
                RegExpr,
                StringDetect,
                WordDetect,
                keyword,
            };

            Type type{};

            bool isDotRegex = false;
            int line = -1;

            // commonAttributes
            QString attribute;
            ContextName context;
            QString beginRegion;
            QString endRegion;
            int column = -1;
            XmlBool lookAhead{};
            XmlBool firstNonSpace{};

            // StringDetect, WordDetect, keyword
            XmlBool insensitive{};

            // DetectChar, StringDetect, RegExpr, keyword
            XmlBool dynamic{};

            // Regex
            XmlBool minimal{};

            // IncludeRule
            XmlBool includeAttrib{};

            // DetectChar, Detect2Chars, LineContinue, RangeDetect
            QChar char0;
            // Detect2Chars, RangeDetect
            QChar char1;

            // AnyChar, StringDetect, RegExpr, WordDetect, keyword
            QString string;
            // RegExpr without .* as suffix
            QString sanitizedString;

            // Float, HlCHex, HlCOct, Int, WordDetect, keyword
            QString additionalDeliminator;
            QString weakDeliminator;

            // rules included by IncludeRules (without IncludeRule)
            QList<const Rule *> includedRules;

            // IncludeRules included by IncludeRules
            QSet<const Rule *> includedIncludeRules;

            Context const *parentContext = nullptr;

            QString filename;

            bool parseElement(const QString &filename, const QXmlStreamReader &xml)
            {
                this->filename = filename;
                line = xml.lineNumber();

                using Pair = QPair<QStringView, Type>;
                static const auto pairs = {
                    Pair{u"AnyChar"_sv, Type::AnyChar},
                    Pair{u"Detect2Chars"_sv, Type::Detect2Chars},
                    Pair{u"DetectChar"_sv, Type::DetectChar},
                    Pair{u"DetectIdentifier"_sv, Type::DetectIdentifier},
                    Pair{u"DetectSpaces"_sv, Type::DetectSpaces},
                    Pair{u"Float"_sv, Type::Float},
                    Pair{u"HlCChar"_sv, Type::HlCChar},
                    Pair{u"HlCHex"_sv, Type::HlCHex},
                    Pair{u"HlCOct"_sv, Type::HlCOct},
                    Pair{u"HlCStringChar"_sv, Type::HlCStringChar},
                    Pair{u"IncludeRules"_sv, Type::IncludeRules},
                    Pair{u"Int"_sv, Type::Int},
                    Pair{u"LineContinue"_sv, Type::LineContinue},
                    Pair{u"RangeDetect"_sv, Type::RangeDetect},
                    Pair{u"RegExpr"_sv, Type::RegExpr},
                    Pair{u"StringDetect"_sv, Type::StringDetect},
                    Pair{u"WordDetect"_sv, Type::WordDetect},
                    Pair{u"keyword", Type::keyword},
                };

                for (auto pair : pairs) {
                    if (xml.name() == pair.first) {
                        type = pair.second;
                        bool success = parseAttributes(filename, xml);
                        success = checkMandoryAttributes(filename, xml) && success;
                        if (success && type == Type::RegExpr) {
                            // ., (.) followed by *, +, {1} or nothing
                            static const QRegularExpression isDot(QStringLiteral(R"(^\(?\.(?:[*+][*+?]?|[*+]|\{1\})?\$?$)"));
                            // remove "(?:" and ")"
                            static const QRegularExpression removeParentheses(QStringLiteral(R"(\((?:\?:)?|\))"));
                            // remove parentheses on a copy of string
                            auto reg = QString(string).replace(removeParentheses, QString());
                            isDotRegex = reg.contains(isDot);

                            // Remove .* and .*$ suffix.
                            static const QRegularExpression allSuffix(QStringLiteral("(?<!\\\\)[.][*][?+]?[$]?$"));
                            sanitizedString = string;
                            sanitizedString.replace(allSuffix, QString());
                            // string is a catch-all, do not sanitize
                            if (sanitizedString.isEmpty() || sanitizedString == u"^"_sv) {
                                sanitizedString = string;
                            }
                        }
                        return success;
                    }
                }

                qWarning() << filename << "line" << xml.lineNumber() << "unknown element:" << xml.name();
                return false;
            }

        private:
            bool parseAttributes(const QString &filename, const QXmlStreamReader &xml)
            {
                bool success = true;

                const auto attrs = xml.attributes();
                for (const auto &attr : attrs) {
                    Parser parser{filename, xml, attr, success};

                    // clang-format off
                    const bool isExtracted
                        = parser.extractString(attribute, u"attribute"_sv)
                       || parser.extractString(context.name, u"context"_sv)
                       || parser.extractXmlBool(lookAhead, u"lookAhead"_sv)
                       || parser.extractXmlBool(firstNonSpace, u"firstNonSpace"_sv)
                       || parser.extractString(beginRegion, u"beginRegion"_sv)
                       || parser.extractString(endRegion, u"endRegion"_sv)
                       || parser.extractPositive(column, u"column"_sv)
                       || ((type == Type::RegExpr
                         || type == Type::StringDetect
                         || type == Type::WordDetect
                         || type == Type::keyword
                         ) && parser.extractXmlBool(insensitive, u"insensitive"_sv))
                       || ((type == Type::DetectChar
                         || type == Type::RegExpr
                         || type == Type::StringDetect
                         || type == Type::keyword
                         ) && parser.extractXmlBool(dynamic, u"dynamic"_sv))
                       || ((type == Type::RegExpr)
                           && parser.extractXmlBool(minimal, u"minimal"_sv))
                       || ((type == Type::DetectChar
                         || type == Type::Detect2Chars
                         || type == Type::LineContinue
                         || type == Type::RangeDetect
                         ) && parser.extractChar(char0, u"char"_sv))
                       || ((type == Type::Detect2Chars
                         || type == Type::RangeDetect
                         ) && parser.extractChar(char1, u"char1"_sv))
                       || ((type == Type::AnyChar
                         || type == Type::RegExpr
                         || type == Type::StringDetect
                         || type == Type::WordDetect
                         || type == Type::keyword
                         ) && parser.extractString(string, u"String"_sv))
                       || ((type == Type::IncludeRules)
                           && parser.extractXmlBool(includeAttrib, u"includeAttrib"_sv))
                       || ((type == Type::Float
                         || type == Type::HlCHex
                         || type == Type::HlCOct
                         || type == Type::Int
                         || type == Type::keyword
                         || type == Type::WordDetect
                         ) && (parser.extractString(additionalDeliminator, u"additionalDeliminator"_sv)
                            || parser.extractString(weakDeliminator, u"weakDeliminator"_sv)))
                    ;
                    // clang-format on

                    success = parser.checkIfExtracted(isExtracted);
                }

                if (type == Type::LineContinue && char0 == u'\0') {
                    char0 = u'\\';
                }

                return success;
            }

            bool checkMandoryAttributes(const QString &filename, const QXmlStreamReader &xml)
            {
                QString missingAttr;

                switch (type) {
                case Type::Unknown:
                    return false;

                case Type::AnyChar:
                case Type::RegExpr:
                case Type::StringDetect:
                case Type::WordDetect:
                case Type::keyword:
                    missingAttr = string.isEmpty() ? QStringLiteral("String") : QString();
                    break;

                case Type::DetectChar:
                    missingAttr = !char0.unicode() ? QStringLiteral("char") : QString();
                    break;

                case Type::Detect2Chars:
                case Type::RangeDetect:
                    missingAttr = !char0.unicode() && !char1.unicode() ? QStringLiteral("char and char1")
                        : !char0.unicode()                             ? QStringLiteral("char")
                        : !char1.unicode()                             ? QStringLiteral("char1")
                                                                       : QString();
                    break;

                case Type::IncludeRules:
                    missingAttr = context.name.isEmpty() ? QStringLiteral("context") : QString();
                    break;

                case Type::DetectIdentifier:
                case Type::DetectSpaces:
                case Type::Float:
                case Type::HlCChar:
                case Type::HlCHex:
                case Type::HlCOct:
                case Type::HlCStringChar:
                case Type::Int:
                case Type::LineContinue:
                    break;
                }

                if (!missingAttr.isEmpty()) {
                    qWarning() << filename << "line" << xml.lineNumber() << "missing attribute:" << missingAttr;
                    return false;
                }

                return true;
            }
        };

        int line;
        // becomes false when a context (except includeRule) refers to it
        bool isOnlyIncluded = true;
        // becomes true when an includedRule refers to it with includeAttrib=true
        bool referencedWithIncludeAttrib = false;
        bool hasDynamicRule = false;
        QString name;
        QString attribute;
        ContextName lineEndContext;
        ContextName lineEmptyContext;
        ContextName fallthroughContext;
        QList<Rule> rules;
        XmlBool dynamic{};
        XmlBool fallthrough{};
        XmlBool stopEmptyLineContextSwitchLoop{};

        bool parseElement(const QString &filename, const QXmlStreamReader &xml)
        {
            line = xml.lineNumber();

            bool success = true;

            const auto attrs = xml.attributes();
            for (const auto &attr : attrs) {
                Parser parser{filename, xml, attr, success};
                XmlBool noIndentationBasedFolding{};

                // clang-format off
                const bool isExtracted = parser.extractString(name, u"name"_sv)
                    || parser.extractString(attribute, u"attribute"_sv)
                    || parser.extractString(lineEndContext.name, u"lineEndContext"_sv)
                    || parser.extractString(lineEmptyContext.name, u"lineEmptyContext"_sv)
                    || parser.extractString(fallthroughContext.name, u"fallthroughContext"_sv)
                    || parser.extractXmlBool(dynamic, u"dynamic"_sv)
                    || parser.extractXmlBool(fallthrough, u"fallthrough"_sv)
                    || parser.extractXmlBool(stopEmptyLineContextSwitchLoop, u"stopEmptyLineContextSwitchLoop"_sv)
                    || parser.extractXmlBool(noIndentationBasedFolding, u"noIndentationBasedFolding"_sv);
                // clang-format on

                success = parser.checkIfExtracted(isExtracted);
            }

            if (name.isEmpty()) {
                qWarning() << filename << "line" << xml.lineNumber() << "missing attribute: name";
                success = false;
            }

            if (attribute.isEmpty()) {
                qWarning() << filename << "line" << xml.lineNumber() << "missing attribute: attribute";
                success = false;
            }

            return success;
        }
    };

    struct ItemDatas {
        struct Style {
            QString name;
            int line;

            friend size_t qHash(const Style &style, size_t seed = 0)
            {
                return qHash(style.name, seed);
            }

            friend bool operator==(const Style &style0, const Style &style1)
            {
                return style0.name == style1.name;
            }
        };

        QSet<Style> styleNames;

        bool parseElement(const QString &filename, const QXmlStreamReader &xml)
        {
            bool success = true;

            QString name;
            QString defStyleNum;
            XmlBool boolean;

            const auto attrs = xml.attributes();
            for (const auto &attr : attrs) {
                Parser parser{filename, xml, attr, success};

                // clang-format off
                const bool isExtracted
                    = parser.extractString(name, u"name"_sv)
                   || parser.extractString(defStyleNum, u"defStyleNum"_sv)
                   || parser.extractXmlBool(boolean, u"bold"_sv)
                   || parser.extractXmlBool(boolean, u"italic"_sv)
                   || parser.extractXmlBool(boolean, u"underline"_sv)
                   || parser.extractXmlBool(boolean, u"strikeOut"_sv)
                   || parser.extractXmlBool(boolean, u"spellChecking"_sv)
                   || parser.checkColor(u"color"_sv)
                   || parser.checkColor(u"selColor"_sv)
                   || parser.checkColor(u"backgroundColor"_sv)
                   || parser.checkColor(u"selBackgroundColor"_sv);
                // clang-format on

                success = parser.checkIfExtracted(isExtracted);
            }

            if (!name.isEmpty()) {
                const auto len = styleNames.size();
                styleNames.insert({name, int(xml.lineNumber())});
                if (len == styleNames.size()) {
                    qWarning() << filename << "line" << xml.lineNumber() << "itemData duplicate:" << name;
                    success = false;
                }
            }

            return success;
        }
    };

    struct Definition {
        QMap<QString, Keywords> keywordsList;
        QMap<QString, Context> contexts;
        ItemDatas itemDatas;
        QString firstContextName;
        const Context *firstContext = nullptr;
        QString filename;
        WordDelimiters wordDelimiters;
        KateVersion kateVersion{};
        QString kateVersionStr;
        QString languageName;
        QSet<const Definition *> referencedDefinitions;

        // Parse <keywords ...>
        bool parseKeywords(const QXmlStreamReader &xml)
        {
            wordDelimiters.append(xml.attributes().value(u"additionalDeliminator"_sv));
            wordDelimiters.remove(xml.attributes().value(u"weakDeliminator"_sv));
            return true;
        }
    };

    // Parse <context>
    void processContextElement(const QXmlStreamReader &xml)
    {
        Context context;
        m_success = context.parseElement(m_currentDefinition->filename, xml) && m_success;
        if (m_currentDefinition->firstContextName.isEmpty()) {
            m_currentDefinition->firstContextName = context.name;
        }
        if (m_currentDefinition->contexts.contains(context.name)) {
            qWarning() << m_currentDefinition->filename << "line" << xml.lineNumber() << "duplicate context:" << context.name;
            m_success = false;
        }
        m_currentContext = &*m_currentDefinition->contexts.insert(context.name, context);
    }

    // Parse <list name="...">
    void processListElement(const QXmlStreamReader &xml)
    {
        Keywords keywords;
        m_success = keywords.parseElement(m_currentDefinition->filename, xml) && m_success;
        if (m_currentDefinition->keywordsList.contains(keywords.name)) {
            qWarning() << m_currentDefinition->filename << "line" << xml.lineNumber() << "duplicate list:" << keywords.name;
            m_success = false;
        }
        m_currentKeywords = &*m_currentDefinition->keywordsList.insert(keywords.name, keywords);
    }

    const Definition *maxKateVersionDefinition(const Definition &definition, QMap<const Definition *, const Definition *> &maxVersionByDefinitions) const
    {
        auto it = maxVersionByDefinitions.find(&definition);
        if (it != maxVersionByDefinitions.end()) {
            return it.value();
        } else {
            auto it = maxVersionByDefinitions.insert(&definition, &definition);
            for (const auto &referencedDef : definition.referencedDefinitions) {
                auto *maxDef = maxKateVersionDefinition(*referencedDef, maxVersionByDefinitions);
                if (it.value()->kateVersion < maxDef->kateVersion) {
                    it.value() = maxDef;
                }
            }
            return it.value();
        }
    }

    // Initialize the referenced rules (Rule::includedRules)
    void resolveIncludeRules()
    {
        QSet<const Context *> usedContexts;
        QList<const Context *> contexts;

        QMutableMapIterator<QString, Definition> def(m_definitions);
        while (def.hasNext()) {
            def.next();
            auto &definition = def.value();
            QMutableMapIterator<QString, Context> contextIt(definition.contexts);
            while (contextIt.hasNext()) {
                contextIt.next();
                auto &currentContext = contextIt.value();
                for (auto &rule : currentContext.rules) {
                    if (rule.type != Context::Rule::Type::IncludeRules) {
                        continue;
                    }

                    if (rule.context.stay) {
                        qWarning() << definition.filename << "line" << rule.line << "IncludeRules refers to himself";
                        m_success = false;
                        continue;
                    }

                    if (rule.context.popCount) {
                        qWarning() << definition.filename << "line" << rule.line << "IncludeRules with #pop prefix";
                        m_success = false;
                    }

                    if (!rule.context.context) {
                        m_success = false;
                        continue;
                    }

                    // resolve includedRules and includedIncludeRules

                    usedContexts.clear();
                    usedContexts.insert(rule.context.context);
                    contexts.clear();
                    contexts.append(rule.context.context);

                    for (int i = 0; i < contexts.size(); ++i) {
                        currentContext.hasDynamicRule = contexts[i]->hasDynamicRule;
                        for (const auto &includedRule : contexts[i]->rules) {
                            if (includedRule.type != Context::Rule::Type::IncludeRules) {
                                rule.includedRules.append(&includedRule);
                            } else if (&rule == &includedRule) {
                                qWarning() << definition.filename << "line" << rule.line << "IncludeRules refers to himself by recursivity";
                                m_success = false;
                            } else {
                                rule.includedIncludeRules.insert(&includedRule);

                                if (includedRule.includedRules.isEmpty()) {
                                    const auto *context = includedRule.context.context;
                                    if (context && !usedContexts.contains(context)) {
                                        contexts.append(context);
                                        usedContexts.insert(context);
                                    }
                                } else {
                                    rule.includedRules.append(includedRule.includedRules);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //! Recursively extracts the contexts used from the first context of the definitions.
    //! This method detects groups of contexts which are only used among themselves.
    QSet<const Context *> extractUsedContexts() const
    {
        QSet<const Context *> usedContexts;
        QList<const Context *> contexts;

        QMapIterator<QString, Definition> def(m_definitions);
        while (def.hasNext()) {
            def.next();
            const auto &definition = def.value();

            if (definition.firstContext) {
                usedContexts.insert(definition.firstContext);
                contexts.clear();
                contexts.append(definition.firstContext);

                for (int i = 0; i < contexts.size(); ++i) {
                    auto appendContext = [&](const Context *context) {
                        if (context && !usedContexts.contains(context)) {
                            contexts.append(context);
                            usedContexts.insert(context);
                        }
                    };

                    const auto *context = contexts[i];
                    appendContext(context->lineEndContext.context);
                    appendContext(context->lineEmptyContext.context);
                    appendContext(context->fallthroughContext.context);

                    for (auto &rule : context->rules) {
                        appendContext(rule.context.context);
                    }
                }
            }
        }

        return usedContexts;
    }

    struct RuleAndInclude {
        const Context::Rule *rule;
        const Context::Rule *includeRules;

        explicit operator bool() const
        {
            return rule;
        }
    };

    struct IncludedRuleUnreachableBy {
        QList<RuleAndInclude> unreachableBy;
        bool alwaysUnreachable = true;
    };

    //! Check contexts and rules
    bool checkContexts(const Definition &definition,
                       QSet<ItemDatas::Style> &usedAttributeNames,
                       QSet<ItemDatas::Style> &ignoredAttributeNames,
                       const QSet<const Context *> &usedContexts,
                       QMap<const Context::Rule *, IncludedRuleUnreachableBy> &unreachableIncludedRules) const
    {
        bool success = true;

        QMapIterator<QString, Context> contextIt(definition.contexts);
        while (contextIt.hasNext()) {
            contextIt.next();

            const auto &context = contextIt.value();
            const auto &filename = definition.filename;

            if (!usedContexts.contains(&context)) {
                qWarning() << filename << "line" << context.line << "unused context:" << context.name;
                success = false;
                continue;
            }

            if (context.name.startsWith(u"#pop"_sv)) {
                qWarning() << filename << "line" << context.line << "the context name must not start with '#pop':" << context.name;
                success = false;
            }

            if (!context.attribute.isEmpty() && (!context.isOnlyIncluded || context.referencedWithIncludeAttrib)) {
                usedAttributeNames.insert({context.attribute, context.line});
            }

            success = checkContextAttribute(definition, context) && success;
            success = checkUreachableRules(definition.filename, context, unreachableIncludedRules) && success;
            success = suggestRuleMerger(definition.filename, context) && success;

            for (const auto &rule : context.rules) {
                if (!rule.attribute.isEmpty()) {
                    if (rule.lookAhead != XmlBool::True) {
                        usedAttributeNames.insert({rule.attribute, rule.line});
                    } else {
                        ignoredAttributeNames.insert({rule.attribute, rule.line});
                    }
                }
                success = checkLookAhead(rule) && success;
                success = checkStringDetect(rule) && success;
                success = checkWordDetect(rule) && success;
                success = checkKeyword(definition, rule) && success;
                success = checkRegExpr(filename, rule, context) && success;
                success = checkDelimiters(definition, rule) && success;
            }
        }

        return success;
    }

    //! Check that a regular expression in a RegExpr rule:
    //! - isValid()
    //! - character ranges such as [A-Z] are valid and not accidentally e.g. [A-z].
    //! - dynamic=true but no place holder used?
    //! - is not . with lookAhead="1"
    //! - is not ^... without column ou firstNonSpace attribute
    //! - is not equivalent to DetectSpaces, DetectChar, Detect2Chars, StringDetect, DetectIdentifier, RangeDetect, LineContinue or AnyChar
    //! - has no unused captures
    //! - has no unnecessary quantifier with lookAhead
    bool checkRegExpr(const QString &filename, const Context::Rule &rule, const Context &context) const
    {
        // ignore empty regex because the error is raised during xml parsing
        if (rule.type == Context::Rule::Type::RegExpr && !rule.string.isEmpty()) {
            const QRegularExpression regexp(rule.string);
            if (!checkRegularExpression(rule.filename, regexp, rule.line)) {
                return false;
            }

            // dynamic == true and no place holder?
            if (rule.dynamic == XmlBool::True) {
                static const QRegularExpression placeHolder(QStringLiteral("%\\d+"));
                if (!rule.string.contains(placeHolder)) {
                    qWarning() << rule.filename << "line" << rule.line << "broken regex:" << rule.string << "problem: dynamic=true but no %\\d+ placeholder";
                    return false;
                }
            }

            if (rule.lookAhead == XmlBool::True && (rule.string.endsWith(u".*$"_sv) || rule.string.endsWith(u".*"_sv)) && -1 == rule.string.indexOf(u'|')) {
                qWarning() << rule.filename << "line" << rule.line << "RegExpr with lookAhead=1 doesn't need to end with '.*' or '.*$':" << rule.string;
                return false;
            }

            auto reg = (rule.lookAhead == XmlBool::True) ? rule.sanitizedString : rule.string;
            if (rule.lookAhead == XmlBool::True) {
                static const QRegularExpression removeAllSuffix(QStringLiteral(
                    R"(((?<!\\)\\(?:[DSWdsw]|x[0-9a-fA-F]{2}|x\{[0-9a-fA-F]+\}|0\d\d|o\{[0-7]+\}|u[0-9a-fA-F]{4})|(?<!\\)[^])}\\]|(?=\\)\\\\)[*][?+]?$)"));
                reg.replace(removeAllSuffix, QString());
            }

            reg.replace(QStringLiteral("{1}"), QString());
            reg.replace(QStringLiteral("{1,1}"), QString());

            // is DetectSpaces
            // optional ^ then \s, [\s], [\t ], [ \t] possibly in (...) or (?:...) followed by *, +
            static const QRegularExpression isDetectSpaces(
                QStringLiteral(R"(^\^?(?:\((?:\?:)?)?\^?(?:\\s|\[(?:\\s| (?:\t|\\t)|(?:\t|\\t) )\])\)?(?:[*+][*+?]?|[*+])?\)?\)?$)"));
            if (rule.string.contains(isDetectSpaces)) {
                char const *extraMsg = rule.string.contains(u'^') ? "+ column=\"0\" or firstNonSpace=\"1\"" : "";
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by DetectSpaces / DetectChar / AnyChar" << extraMsg << ":"
                           << rule.string;
                return false;
            }

#define REG_ESCAPE_CHAR R"(\\(?:[^0BDPSWbdpswoux]|x[0-9a-fA-F]{2}|x\{[0-9a-fA-F]+\}|0\d\d|o\{[0-7]+\}|u[0-9a-fA-F]{4}))"
#define REG_CHAR "(?:" REG_ESCAPE_CHAR "|\\[(?:" REG_ESCAPE_CHAR "|.)\\]|[^[.^])"

            // is RangeDetect
            static const QRegularExpression isRange(QStringLiteral("^\\^?" REG_CHAR "(?:"
                                                                   "\\.\\*[?+]?" REG_CHAR "|"
                                                                   "\\[\\^(" REG_ESCAPE_CHAR "|.)\\]\\*[?+]?\\1"
                                                                   ")$"));
            if ((rule.lookAhead == XmlBool::True || rule.minimal == XmlBool::True || rule.string.contains(u".*?"_sv) || rule.string.contains(u"[^"_sv))
                && reg.contains(isRange)) {
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by RangeDetect:" << rule.string;
                return false;
            }

            // is AnyChar
            static const QRegularExpression isAnyChar(QStringLiteral(R"(^(\^|\((\?:)?)*\[(?!\^)[-\]]?(\\[^0BDPSWbdpswoux]|[^-\]\\])*\]\)*$)"));
            if (rule.string.contains(isAnyChar)) {
                auto extra = (reg[0] == u'^' || reg[1] == u'^') ? "with column=\"0\"" : "";
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by AnyChar:" << rule.string << extra;
                return false;
            }

            // is LineContinue
            static const QRegularExpression isLineContinue(QStringLiteral("^\\^?" REG_CHAR "\\$$"));
            if (reg.contains(isLineContinue)) {
                auto extra = (reg[0] == u'^') ? "with column=\"0\"" : "";
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by LineContinue:" << rule.string << extra;
                return false;
            }

#define REG_DIGIT uR"((\[(0-9|\\d)\]|\\d))"
#define REG_DIGITS REG_DIGIT u"([+]|" REG_DIGIT u"[*])"
#define REG_DOT uR"((\\[.]|\[.\]))"
            // is Int, check \b[0-9]+
            static const QRegularExpression isInt(uR"(^(\((\?:)?)*\\b(\((\?:)?)*)" REG_DIGITS uR"(\)*$)"_s);
            if (reg.contains(isInt)) {
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by Int:" << rule.string;
                return false;
            }

            // is Float, check (\b[0-9]+\.[0-9]*|\.[0-9]+)([eE][-+]?[0-9]+)?
            static const QRegularExpression isFloat(
                uR"(^(\\b|\((\?:)?)*)" REG_DIGITS REG_DOT
                    REG_DIGIT u"[*][|]" REG_DOT REG_DIGITS uR"(\)+\((\?:)?\[[eE]+\]\[(\\?-\\?\+|\\?\+\\?-)\]\?)" REG_DIGITS uR"(\)\?\)*$)"_s);
            if (reg.contains(isFloat)) {
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by Float:" << rule.string;
                return false;
            }
#undef REG_DOT
#undef REG_DIGIT
#undef REG_DIGITS

            // replace \c, \xhhh, \x{hhh...}, \0dd, \o{ddd}, \uhhhh, with _
            static const QRegularExpression sanitize1(QStringLiteral(REG_ESCAPE_CHAR));
            reg.replace(sanitize1, QStringLiteral("_"));

#undef REG_CHAR
#undef REG_ESCAPE_CHAR

            // use minimal or lazy operator
            static const QRegularExpression isMinimal(QStringLiteral("(?![.][*+?][$]?[)]*$)[.][*+?][^?+]"));
            static const QRegularExpression hasNotGreedy(QStringLiteral("[*+?][?+]"));

            if (rule.lookAhead == XmlBool::True && rule.minimal != XmlBool::True && reg.contains(isMinimal) && !reg.contains(hasNotGreedy)
                && (!rule.context.context || !rule.context.context->hasDynamicRule || regexp.captureCount() == 0)
                && (reg.back() != u'$' || reg.contains(u'|'))) {
                qWarning() << rule.filename << "line" << rule.line
                           << "RegExpr should be have minimal=\"1\" or use lazy operator (i.g, '.*' -> '.*?'):" << rule.string;
                return false;
            }

            // replace [:...:] with ___
            static const QRegularExpression sanitize2(QStringLiteral(R"(\[:\w+:\])"));
            reg.replace(sanitize2, QStringLiteral("___"));

            // replace [ccc...], [special] with ...
            static const QRegularExpression sanitize3(QStringLiteral(R"(\[(?:\^\]?[^]]*|\]?[^]\\]*?\\.[^]]*|\][^]]{2,}|[^]]{3,})\]|(\[\]?[^]]*\]))"));
            reg.replace(sanitize3, QStringLiteral("...\\1"));

            // replace [c] with _
            static const QRegularExpression sanitize4(QStringLiteral(R"(\[.\])"));
            reg.replace(sanitize4, QStringLiteral("_"));

            const int len = reg.size();
            // replace [cC] with _
            static const QRegularExpression toInsensitive(QStringLiteral(R"(\[(?:([^]])\1)\])"));
            reg = reg.toUpper();
            reg.replace(toInsensitive, QString());

            // is StringDetect
            // ignore (?:, ) and {n}
            static const QRegularExpression isStringDetect(QStringLiteral(R"(^\^?(?:[^|\\?*+$^[{(.]|{(?!\d+,\d*}|,\d+})|\(\?:)+$)"));
            if (reg.contains(isStringDetect)) {
                char const *extraMsg = rule.string.contains(u'^') ? "+ column=\"0\" or firstNonSpace=\"1\"" : "";
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by StringDetect / Detect2Chars / DetectChar" << extraMsg
                           << ":" << rule.string;
                if (len != reg.size()) {
                    qWarning() << rule.filename << "line" << rule.line << "insensitive=\"1\" missing:" << rule.string;
                }
                return false;
            }

            // column="0"
            if (rule.column == -1) {
                // ^ without |
                // (^sas*) -> ok
                // (^sa|s*) -> ko
                // (^(sa|s*)) -> ok
                auto first = std::as_const(reg).begin();
                auto last = std::as_const(reg).end();
                int depth = 0;

                while (u'(' == *first) {
                    ++depth;
                    ++first;
                    if (u'?' == *first || u':' == first[1]) {
                        first += 2;
                    }
                }

                if (u'^' == *first) {
                    const int bolDepth = depth;
                    bool replace = true;

                    while (++first != last) {
                        if (u'(' == *first) {
                            ++depth;
                        } else if (u')' == *first) {
                            --depth;
                            if (depth < bolDepth) {
                                // (^a)? === (^a|) -> ko
                                if (first + 1 != last && u"*?"_sv.contains(first[1])) {
                                    replace = false;
                                    break;
                                }
                            }
                        } else if (u'|' == *first) {
                            // ignore '|' within subgroup
                            if (depth <= bolDepth) {
                                replace = false;
                                break;
                            }
                        }
                    }

                    if (replace) {
                        qWarning() << rule.filename << "line" << rule.line << "column=\"0\" missing with RegExpr:" << rule.string;
                        return false;
                    }
                }
            }

            // add ^ with column=0
            if (rule.column == 0 && !rule.isDotRegex) {
                bool hasStartOfLine = false;
                auto first = std::as_const(reg).begin();
                auto last = std::as_const(reg).end();
                for (; first != last; ++first) {
                    if (*first == u'^') {
                        hasStartOfLine = true;
                        break;
                    } else if (*first == u'(') {
                        if (last - first >= 3 && first[1] == u'?' && first[2] == u':') {
                            first += 2;
                        }
                    } else {
                        break;
                    }
                }

                if (!hasStartOfLine) {
                    qWarning() << rule.filename << "line" << rule.line
                               << "start of line missing in the pattern with column=\"0\" (i.e. abc -> ^abc):" << rule.string;
                    return false;
                }
            }

            bool useCapture = false;

            // detection of unnecessary capture
            if (regexp.captureCount()) {
                auto maximalCapture = [](const QStringView(&referenceNames)[9], const QString &s) {
                    int maxCapture = 9;
                    while (maxCapture && !s.contains(referenceNames[maxCapture - 1])) {
                        --maxCapture;
                    }
                    return maxCapture;
                };

                int maxCaptureUsed = 0;
                // maximal dynamic reference
                if (rule.context.context && !rule.context.stay) {
                    for (const auto &nextRule : std::as_const(rule.context.context->rules)) {
                        if (nextRule.dynamic == XmlBool::True) {
                            static const QStringView cap[]{
                                u"%1"_sv,
                                u"%2"_sv,
                                u"%3"_sv,
                                u"%4"_sv,
                                u"%5"_sv,
                                u"%6"_sv,
                                u"%7"_sv,
                                u"%8"_sv,
                                u"%9"_sv,
                            };
                            int maxDynamicCapture = maximalCapture(cap, nextRule.string);
                            maxCaptureUsed = std::max(maxCaptureUsed, maxDynamicCapture);
                        }
                    }
                }

                static const QStringView num1[]{
                    u"\\1"_sv,
                    u"\\2"_sv,
                    u"\\3"_sv,
                    u"\\4"_sv,
                    u"\\5"_sv,
                    u"\\6"_sv,
                    u"\\7"_sv,
                    u"\\8"_sv,
                    u"\\9"_sv,
                };
                static const QStringView num2[]{
                    u"\\g1"_sv,
                    u"\\g2"_sv,
                    u"\\g3"_sv,
                    u"\\g4"_sv,
                    u"\\g5"_sv,
                    u"\\g6"_sv,
                    u"\\g7"_sv,
                    u"\\g8"_sv,
                    u"\\g9"_sv,
                };
                const int maxBackReference = std::max(maximalCapture(num1, rule.string), maximalCapture(num2, rule.string));

                const int maxCapture = std::max(maxCaptureUsed, maxBackReference);

                if (maxCapture && regexp.captureCount() > maxCapture) {
                    qWarning() << rule.filename << "line" << rule.line << "RegExpr with" << regexp.captureCount() << "captures but only" << maxCapture
                               << "are used. Please, replace '(...)' with '(?:...)':" << rule.string;
                    return false;
                }

                useCapture = maxCapture;
            }

            if (!useCapture) {
                // is DetectIdentifier
                static const QRegularExpression isDetectIdentifier(
                    QStringLiteral(R"(^(\((\?:)?|\^)*\[(\\p\{L\}|_){2}\]([+][?+]?)?\[(\\p\{N\}|\\p\{L\}|_){3}\][*][?+]?\)*$)"));
                if (rule.string.contains(isDetectIdentifier)) {
                    qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by DetectIdentifier:" << rule.string;
                    return false;
                }
            }

            if (rule.isDotRegex) {
                // search next rule with same column or firstNonSpace
                int i = &rule - context.rules.data() + 1;
                const bool hasColumn = (rule.column != -1);
                const bool hasFirstNonSpace = (rule.firstNonSpace == XmlBool::True);
                const bool isSpecial = (hasColumn || hasFirstNonSpace);
                for (; i < context.rules.size(); ++i) {
                    auto &rule2 = context.rules[i];
                    if (rule2.type == Context::Rule::Type::IncludeRules && isSpecial) {
                        i = context.rules.size();
                        break;
                    }

                    const bool hasColumn2 = (rule2.column != -1);
                    const bool hasFirstNonSpace2 = (rule2.firstNonSpace == XmlBool::True);
                    if ((!isSpecial && !hasColumn2 && !hasFirstNonSpace2) || (hasColumn && rule.column == rule2.column)
                        || (hasFirstNonSpace && hasFirstNonSpace2)) {
                        break;
                    }
                }

                auto ruleFilename = (filename == rule.filename) ? QString() : u"in "_sv + rule.filename;
                if (i == context.rules.size()) {
                    if (rule.lookAhead == XmlBool::True && rule.firstNonSpace != XmlBool::True && rule.column == -1 && rule.beginRegion.isEmpty()
                        && rule.endRegion.isEmpty() && !useCapture) {
                        qWarning() << filename << "context line" << context.line << ": RegExpr line" << rule.line << ruleFilename
                                   << "should be replaced by fallthroughContext:" << rule.string;
                    }
                } else {
                    auto &nextRule = context.rules[i];
                    auto nextRuleFilename = (filename == nextRule.filename) ? QString() : u"in "_sv + nextRule.filename;
                    qWarning() << filename << "context line" << context.line << "contains unreachable element line" << nextRule.line << nextRuleFilename
                               << "because a dot RegExpr is used line" << rule.line << ruleFilename;
                }

                // unnecessary quantifier
                static const QRegularExpression unnecessaryQuantifier1(QStringLiteral(R"([*+?]([.][*+?]{0,2})?$)"));
                static const QRegularExpression unnecessaryQuantifier2(QStringLiteral(R"([*+?]([.][*+?]{0,2})?[)]*$)"));
                auto &unnecessaryQuantifier = useCapture ? unnecessaryQuantifier1 : unnecessaryQuantifier2;
                if (rule.lookAhead == XmlBool::True && rule.minimal != XmlBool::True && reg.contains(unnecessaryQuantifier)) {
                    qWarning() << rule.filename << "line" << rule.line
                               << "Last quantifier is not necessary (i.g., 'xyz*' -> 'xy', 'xyz+.' -> 'xyz.'):" << rule.string;
                    return false;
                }
            }
        }

        return true;
    }

    // Parse and check <emptyLine>
    bool parseEmptyLine(const QString &filename, const QXmlStreamReader &xml)
    {
        bool success = true;

        QString pattern;
        XmlBool casesensitive{};

        const auto attrs = xml.attributes();
        for (auto &attr : attrs) {
            Parser parser{filename, xml, attr, success};

            const bool isExtracted = parser.extractString(pattern, u"regexpr"_sv) || parser.extractXmlBool(casesensitive, u"casesensitive"_sv);

            success = parser.checkIfExtracted(isExtracted);
        }

        if (pattern.isEmpty()) {
            qWarning() << filename << "line" << xml.lineNumber() << "missing attribute: regexpr";
            success = false;
        } else {
            success = checkRegularExpression(filename, QRegularExpression(pattern), xml.lineNumber());
        }

        return success;
    }

    //! Check that a regular expression:
    //! - isValid()
    //! - character ranges such as [A-Z] are valid and not accidentally e.g. [A-z].
    bool checkRegularExpression(const QString &filename, const QRegularExpression &regexp, int line) const
    {
        const auto pattern = regexp.pattern();

        // validate regexp
        if (!regexp.isValid()) {
            qWarning() << filename << "line" << line << "broken regex:" << pattern << "problem:" << regexp.errorString() << "at offset"
                       << regexp.patternErrorOffset();
            return false;
        }

        // catch possible case typos: [A-z] or [a-Z]
        const int azOffset = std::max(pattern.indexOf(u"A-z"_sv), pattern.indexOf(u"a-Z"_sv));
        if (azOffset >= 0) {
            qWarning() << filename << "line" << line << "broken regex:" << pattern << "problem: [a-Z] or [A-z] at offset" << azOffset;
            return false;
        }

        return true;
    }

    //! Check fallthrough and fallthroughContext.
    //! Check kateversion for stopEmptyLineContextSwitchLoop.
    bool checkContextAttribute(const Definition &definition, const Context &context) const
    {
        bool success = true;

        if (!context.fallthroughContext.name.isEmpty()) {
            const bool mandatoryFallthroughAttribute = definition.kateVersion < KateVersion{5, 62};
            if (context.fallthrough == XmlBool::True && !mandatoryFallthroughAttribute) {
                qWarning() << definition.filename << "line" << context.line << "fallthrough attribute is unnecessary with kateversion >= 5.62 in context"
                           << context.name;
                success = false;
            } else if (context.fallthrough != XmlBool::True && mandatoryFallthroughAttribute) {
                qWarning() << definition.filename << "line" << context.line
                           << "fallthroughContext attribute without fallthrough=\"1\" attribute is only valid with kateversion >= 5.62 in context"
                           << context.name;
                success = false;
            }
        }

        if (context.stopEmptyLineContextSwitchLoop != XmlBool::Unspecified && definition.kateVersion < KateVersion{5, 103}) {
            qWarning() << definition.filename << "line" << context.line
                       << "stopEmptyLineContextSwitchLoop attribute is only valid with kateversion >= 5.103 in context" << context.name;
            success = false;
        }

        return success;
    }

    //! Search for additionalDeliminator/weakDeliminator which has no effect.
    bool checkDelimiters(const Definition &definition, const Context::Rule &rule) const
    {
        if (rule.additionalDeliminator.isEmpty() && rule.weakDeliminator.isEmpty()) {
            return true;
        }

        bool success = true;

        if (definition.kateVersion < KateVersion{5, 79}) {
            qWarning() << definition.filename << "line" << rule.line
                       << "additionalDeliminator and weakDeliminator are only available since version \"5.79\". Please, increase kateversion.";
            success = false;
        }

        for (QChar c : rule.additionalDeliminator) {
            if (!definition.wordDelimiters.contains(c)) {
                return success;
            }
        }

        for (QChar c : rule.weakDeliminator) {
            if (definition.wordDelimiters.contains(c)) {
                return success;
            }
        }

        qWarning() << rule.filename << "line" << rule.line << "unnecessary use of additionalDeliminator and/or weakDeliminator" << rule.string;
        return false;
    }

    //! Check that keyword rule reference an existing keyword list.
    bool checkKeyword(const Definition &definition, const Context::Rule &rule) const
    {
        if (rule.type == Context::Rule::Type::keyword) {
            auto it = definition.keywordsList.find(rule.string);
            if (it == definition.keywordsList.end()) {
                qWarning() << rule.filename << "line" << rule.line << "reference of non-existing keyword list:" << rule.string;
                return false;
            }
        }
        return true;
    }

    //! Search for rules with lookAhead="true" and context="#stay".
    //! This would cause an infinite loop.
    bool checkLookAhead(const Context::Rule &rule) const
    {
        if (rule.lookAhead == XmlBool::True && rule.context.stay) {
            qWarning() << rule.filename << "line" << rule.line << "infinite loop: lookAhead with context #stay";
        }
        return true;
    }

    //! Check that StringDetect contains a placeHolder when dynamic="1"
    bool checkStringDetect(const Context::Rule &rule) const
    {
        if (rule.type == Context::Rule::Type::StringDetect) {
            // dynamic == true and no place holder?
            if (rule.dynamic == XmlBool::True) {
                static const QRegularExpression placeHolder(QStringLiteral("%\\d+"));
                if (!rule.string.contains(placeHolder)) {
                    qWarning() << rule.filename << "line" << rule.line << "broken regex:" << rule.string << "problem: dynamic=true but no %\\d+ placeholder";
                    return false;
                }
            }
        }
        return true;
    }

    //! Check that WordDetect does not contain spaces at the beginning and end of text.
    bool checkWordDetect(const Context::Rule &rule) const
    {
        if (rule.type == Context::Rule::Type::WordDetect) {
            if (!rule.string.isEmpty() && (rule.string.front().isSpace() || rule.string.back().isSpace())) {
                qWarning() << rule.filename << "line" << rule.line << "contains a space at the beginning or end of the string:" << rule.string;
                return false;
            }
        }
        return true;
    }

    //! Check \<include> and delimiter in a keyword list
    bool checkKeywordsList(const Definition &definition) const
    {
        bool success = true;

        bool includeNotSupport = (definition.kateVersion < KateVersion{5, 53});
        QMapIterator<QString, Keywords> keywordsIt(definition.keywordsList);
        while (keywordsIt.hasNext()) {
            keywordsIt.next();

            for (const auto &include : keywordsIt.value().items.includes) {
                if (includeNotSupport) {
                    qWarning() << definition.filename << "line" << include.line
                               << "<include> is only available since version \"5.53\". Please, increase kateversion.";
                    success = false;
                }
                success = checkKeywordInclude(definition, include) && success;
            }

            // Check that keyword list items do not have deliminator character
#if 0
            for (const auto& keyword : keywordsIt.value().items.keywords) {
                for (QChar c : keyword.content) {
                    if (definition.wordDelimiters.contains(c)) {
                        qWarning() << definition.filename << "line" << keyword.line << "keyword with delimiter:" << c << "in" << keyword.content;
                        success = false;
                    }
                }
            }
#endif
        }

        return success;
    }

    //! Search for non-existing keyword include.
    bool checkKeywordInclude(const Definition &definition, const Keywords::Items::Item &include) const
    {
        bool containsKeywordName = true;
        int const idx = include.content.indexOf(u"##"_sv);
        if (idx == -1) {
            auto it = definition.keywordsList.find(include.content);
            containsKeywordName = (it != definition.keywordsList.end());
        } else {
            auto defName = include.content.sliced(idx + 2);
            auto listName = include.content.sliced(0, idx);
            auto it = m_definitions.find(defName);
            if (it == m_definitions.end()) {
                qWarning() << definition.filename << "line" << include.line << "unknown definition in" << include.content;
                return false;
            }
            containsKeywordName = it->keywordsList.contains(listName);
        }

        if (!containsKeywordName) {
            qWarning() << definition.filename << "line" << include.line << "unknown keyword name in" << include.content;
        }

        return containsKeywordName;
    }

    //! Check if a rule is hidden by another
    //! - rule hidden by DetectChar or AnyChar
    //! - DetectSpaces, AnyChar, Int, Float with all their characters hidden by DetectChar or AnyChar
    //! - StringDetect, WordDetect, RegExpr with as prefix Detect2Chars or other strings
    //! - duplicate rule (Int, Float, keyword with same String, etc)
    //! - Rule hidden by a dot regex
    bool checkUreachableRules(const QString &filename,
                              const Context &context,
                              QMap<const Context::Rule *, IncludedRuleUnreachableBy> &unreachableIncludedRules) const
    {
        if (context.isOnlyIncluded) {
            return true;
        }

        struct Rule4 {
            RuleAndInclude setRule(const Context::Rule &rule, const Context::Rule *includeRules = nullptr)
            {
                auto set = [&](RuleAndInclude &ruleAndInclude) {
                    auto old = ruleAndInclude;
                    ruleAndInclude = {&rule, includeRules};
                    return old;
                };

                if (rule.firstNonSpace == XmlBool::True) {
                    return set(firstNonSpace);
                } else if (rule.column == 0) {
                    return set(column0);
                } else if (rule.column > 0) {
                    return set(columnGreaterThan0[rule.column]);
                } else {
                    return set(normal);
                }
            }

        private:
            RuleAndInclude normal;
            RuleAndInclude column0;
            QMap<int, RuleAndInclude> columnGreaterThan0;
            RuleAndInclude firstNonSpace;
        };

        // Associate QChar with RuleAndInclude
        struct CharTable {
            /// Search RuleAndInclude associated with @p c.
            RuleAndInclude find(QChar c) const
            {
                if (c.unicode() < 128) {
                    return m_asciiMap[c.unicode()];
                }
                auto it = m_utf8Map.find(c);
                return it == m_utf8Map.end() ? RuleAndInclude{nullptr, nullptr} : it.value();
            }

            /// Search RuleAndInclude associated with the characters of @p s.
            /// \return an empty QList when at least one character is not found.
            QList<RuleAndInclude> find(QStringView s) const
            {
                QList<RuleAndInclude> result;

                for (QChar c : s) {
                    if (!find(c)) {
                        return result;
                    }
                }

                for (QChar c : s) {
                    result.append(find(c));
                }

                return result;
            }

            /// Associates @p c with a rule.
            void append(QChar c, const Context::Rule &rule, const Context::Rule *includeRule = nullptr)
            {
                if (c.unicode() < 128) {
                    m_asciiMap[c.unicode()] = {&rule, includeRule};
                } else {
                    m_utf8Map[c] = {&rule, includeRule};
                }
            }

            /// Associates each character of @p s with a rule.
            void append(QStringView s, const Context::Rule &rule, const Context::Rule *includeRule = nullptr)
            {
                for (QChar c : s) {
                    append(c, rule, includeRule);
                }
            }

        private:
            RuleAndInclude m_asciiMap[127]{};
            QMap<QChar, RuleAndInclude> m_utf8Map;
        };

        struct Char4Tables {
            CharTable chars;
            CharTable charsColumn0;
            QMap<int, CharTable> charsColumnGreaterThan0;
            CharTable charsFirstNonSpace;
        };

        // View on Char4Tables members
        struct CharTableArray {
            // Append Char4Tables members that satisfies firstNonSpace and column.
            // Char4Tables::char is always added.
            CharTableArray(Char4Tables &tables, const Context::Rule &rule)
            {
                if (rule.firstNonSpace == XmlBool::True) {
                    appendTable(tables.charsFirstNonSpace);
                }

                if (rule.column == 0) {
                    appendTable(tables.charsColumn0);
                } else if (rule.column > 0) {
                    appendTable(tables.charsColumnGreaterThan0[rule.column]);
                }

                appendTable(tables.chars);
            }

            // Removes Char4Tables::chars when the rule contains firstNonSpace or column
            void removeNonSpecialWhenSpecial()
            {
                if (m_size > 1) {
                    --m_size;
                }
            }

            /// Search RuleAndInclude associated with @p c.
            RuleAndInclude find(QChar c) const
            {
                for (int i = 0; i < m_size; ++i) {
                    if (auto ruleAndInclude = m_charTables[i]->find(c)) {
                        return ruleAndInclude;
                    }
                }
                return RuleAndInclude{nullptr, nullptr};
            }

            /// Search RuleAndInclude associated with the characters of @p s.
            /// \return an empty QList when at least one character is not found.
            QList<RuleAndInclude> find(QStringView s) const
            {
                for (int i = 0; i < m_size; ++i) {
                    auto result = m_charTables[i]->find(s);
                    if (result.size()) {
                        while (++i < m_size) {
                            result.append(m_charTables[i]->find(s));
                        }
                        return result;
                    }
                }
                return QList<RuleAndInclude>();
            }

            /// Associates @p c with a rule.
            void append(QChar c, const Context::Rule &rule, const Context::Rule *includeRule = nullptr)
            {
                for (int i = 0; i < m_size; ++i) {
                    m_charTables[i]->append(c, rule, includeRule);
                }
            }

            /// Associates each character of @p s with a rule.
            void append(QStringView s, const Context::Rule &rule, const Context::Rule *includeRule = nullptr)
            {
                for (int i = 0; i < m_size; ++i) {
                    m_charTables[i]->append(s, rule, includeRule);
                }
            }

        private:
            void appendTable(CharTable &t)
            {
                m_charTables[m_size] = &t;
                ++m_size;
            }

            CharTable *m_charTables[3];
            int m_size = 0;
        };

        struct ObservableRule {
            const Context::Rule *rule;
            const Context::Rule *includeRules;

            bool hasResolvedIncludeRules() const
            {
                return rule == includeRules;
            }
        };

        // Iterates over all the rules, including those in includedRules
        struct RuleIterator {
            RuleIterator(const QList<ObservableRule> &rules, const ObservableRule &endRule)
                : m_end(&endRule - rules.data())
                , m_rules(rules)
            {
            }

            /// \return next rule or nullptr
            const Context::Rule *next()
            {
                // if in includedRules
                if (m_includedRules) {
                    ++m_i2;
                    if (m_i2 != m_includedRules->size()) {
                        return (*m_includedRules)[m_i2];
                    }
                    ++m_i;
                    m_includedRules = nullptr;
                }

                // if is a includedRules
                while (m_i < m_end && m_rules[m_i].rule->type == Context::Rule::Type::IncludeRules) {
                    if (!m_rules[m_i].includeRules && m_rules[m_i].rule->includedRules.size()) {
                        m_i2 = 0;
                        m_includedRules = &m_rules[m_i].rule->includedRules;
                        return (*m_includedRules)[m_i2];
                    }
                    ++m_i;
                }

                if (m_i < m_end) {
                    ++m_i;
                    return m_rules[m_i - 1].rule;
                }

                return nullptr;
            }

            /// \return current IncludeRules or nullptr
            const Context::Rule *currentIncludeRules() const
            {
                return m_includedRules ? m_rules[m_i].rule : m_rules[m_i].includeRules;
            }

        private:
            int m_i = 0;
            int m_i2 = 0;
            const int m_end;
            const QList<ObservableRule> &m_rules;
            const QList<const Context::Rule *> *m_includedRules = nullptr;
        };

        // Dot regex container that satisfies firstNonSpace and column.
        struct DotRegex {
            /// Append a dot regex rule.
            void append(const Context::Rule &rule, const Context::Rule *includedRule)
            {
                auto array = extractDotRegexes(rule);
                if (array[0]) {
                    *array[0] = {&rule, includedRule};
                }
                if (array[1]) {
                    *array[1] = {&rule, includedRule};
                }
            }

            /// Search dot regex which hides @p rule
            RuleAndInclude find(const Context::Rule &rule)
            {
                auto array = extractDotRegexes(rule);
                if (array[0]) {
                    return *array[0];
                }
                if (array[1]) {
                    return *array[1];
                }
                return RuleAndInclude{};
            }

        private:
            using Array = std::array<RuleAndInclude *, 2>;

            Array extractDotRegexes(const Context::Rule &rule)
            {
                Array ret{};

                if (rule.firstNonSpace != XmlBool::True && rule.column == -1) {
                    ret[0] = &dotRegex;
                } else {
                    if (rule.firstNonSpace == XmlBool::True) {
                        ret[0] = &dotRegexFirstNonSpace;
                    }

                    if (rule.column == 0) {
                        ret[1] = &dotRegexColumn0;
                    } else if (rule.column > 0) {
                        ret[1] = &dotRegexColumnGreaterThan0[rule.column];
                    }
                }

                return ret;
            }

            RuleAndInclude dotRegex{};
            RuleAndInclude dotRegexColumn0{};
            QMap<int, RuleAndInclude> dotRegexColumnGreaterThan0{};
            RuleAndInclude dotRegexFirstNonSpace{};
        };

        bool success = true;

        // characters of DetectChar/AnyChar
        Char4Tables detectChars;
        // characters of dynamic DetectChar
        Char4Tables dynamicDetectChars;
        // characters of LineContinue
        Char4Tables lineContinueChars;

        Rule4 intRule{};
        Rule4 floatRule{};
        Rule4 hlCCharRule{};
        Rule4 hlCOctRule{};
        Rule4 hlCHexRule{};
        Rule4 hlCStringCharRule{};
        Rule4 detectIdentifierRule{};

        // Contains includedRules and included includedRules
        QMap<Context const *, RuleAndInclude> includeContexts;

        DotRegex dotRegex;

        QList<ObservableRule> observedRules;
        observedRules.reserve(context.rules.size());
        for (const Context::Rule &rule : context.rules) {
            const Context::Rule *includeRule = nullptr;
            if (rule.type == Context::Rule::Type::IncludeRules) {
                auto *context = rule.context.context;
                if (context && context->isOnlyIncluded) {
                    includeRule = &rule;
                }
            }

            observedRules.push_back({&rule, includeRule});
            if (includeRule) {
                for (const Context::Rule *rule2 : rule.includedRules) {
                    observedRules.push_back({rule2, includeRule});
                }
            }
        }

        for (auto &observedRule : observedRules) {
            const Context::Rule &rule = *observedRule.rule;
            bool isUnreachable = false;
            QList<RuleAndInclude> unreachableBy;

            // declare rule as unreachable if ruleAndInclude is not empty
            auto updateUnreachable1 = [&](RuleAndInclude ruleAndInclude) {
                if (ruleAndInclude) {
                    isUnreachable = true;
                    unreachableBy.append(ruleAndInclude);
                }
            };

            // declare rule as unreachable if ruleAndIncludes is not empty
            auto updateUnreachable2 = [&](const QList<RuleAndInclude> &ruleAndIncludes) {
                if (!ruleAndIncludes.isEmpty()) {
                    isUnreachable = true;
                    unreachableBy.append(ruleAndIncludes);
                }
            };

            // check if rule2.firstNonSpace/column is compatible with those of rule
            auto isCompatible = [&rule](Context::Rule const &rule2) {
                return (rule2.firstNonSpace != XmlBool::True && rule2.column == -1) || (rule.column == rule2.column && rule.column != -1)
                    || (rule.firstNonSpace == rule2.firstNonSpace && rule.firstNonSpace == XmlBool::True);
            };

            updateUnreachable1(dotRegex.find(rule));

            switch (rule.type) {
            // checks if hidden by DetectChar/AnyChar
            // then add the characters to detectChars
            case Context::Rule::Type::AnyChar: {
                auto tables = CharTableArray(detectChars, rule);
                updateUnreachable2(tables.find(rule.string));
                tables.removeNonSpecialWhenSpecial();
                tables.append(rule.string, rule);
                break;
            }

            // check if is hidden by DetectChar/AnyChar
            // then add the characters to detectChars or dynamicDetectChars
            case Context::Rule::Type::DetectChar: {
                auto &chars4 = (rule.dynamic != XmlBool::True) ? detectChars : dynamicDetectChars;
                auto tables = CharTableArray(chars4, rule);
                updateUnreachable1(tables.find(rule.char0));
                tables.removeNonSpecialWhenSpecial();
                tables.append(rule.char0, rule);
                break;
            }

            // check if hidden by DetectChar/AnyChar
            // then add spaces characters to detectChars
            case Context::Rule::Type::DetectSpaces: {
                auto tables = CharTableArray(detectChars, rule);
                updateUnreachable2(tables.find(u" \t"_sv));
                tables.removeNonSpecialWhenSpecial();
                tables.append(u' ', rule);
                tables.append(u'\t', rule);
                break;
            }

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::HlCChar:
                updateUnreachable1(CharTableArray(detectChars, rule).find(u'\''));
                updateUnreachable1(hlCCharRule.setRule(rule));
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::HlCHex:
                updateUnreachable1(CharTableArray(detectChars, rule).find(u'0'));
                updateUnreachable1(hlCHexRule.setRule(rule));
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::HlCOct:
                updateUnreachable1(CharTableArray(detectChars, rule).find(u'0'));
                updateUnreachable1(hlCOctRule.setRule(rule));
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::HlCStringChar:
                updateUnreachable1(CharTableArray(detectChars, rule).find(u'\\'));
                updateUnreachable1(hlCStringCharRule.setRule(rule));
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::Int:
                updateUnreachable2(CharTableArray(detectChars, rule).find(u"0123456789"_sv));
                updateUnreachable1(intRule.setRule(rule));
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::Float:
                updateUnreachable2(CharTableArray(detectChars, rule).find(u"0123456789."_sv));
                updateUnreachable1(floatRule.setRule(rule));
                // check that Float is before Int
                updateUnreachable1(Rule4(intRule).setRule(rule));
                break;

            // check if hidden by another DetectIdentifier rule
            case Context::Rule::Type::DetectIdentifier:
                updateUnreachable1(detectIdentifierRule.setRule(rule));
                break;

            // check if hidden by DetectChar/AnyChar or another LineContinue
            case Context::Rule::Type::LineContinue: {
                updateUnreachable1(CharTableArray(detectChars, rule).find(rule.char0));

                auto tables = CharTableArray(lineContinueChars, rule);
                updateUnreachable1(tables.find(rule.char0));
                tables.removeNonSpecialWhenSpecial();
                tables.append(rule.char0, rule);
                break;
            }

            // check if hidden by DetectChar/AnyChar or another Detect2Chars/RangeDetect
            case Context::Rule::Type::Detect2Chars:
            case Context::Rule::Type::RangeDetect:
                updateUnreachable1(CharTableArray(detectChars, rule).find(rule.char0));
                if (!isUnreachable) {
                    RuleIterator ruleIterator(observedRules, observedRule);
                    while (const auto *rulePtr = ruleIterator.next()) {
                        if (isUnreachable) {
                            break;
                        }
                        const auto &rule2 = *rulePtr;
                        if (rule2.type == rule.type && isCompatible(rule2) && rule.char0 == rule2.char0 && rule.char1 == rule2.char1) {
                            updateUnreachable1({&rule2, ruleIterator.currentIncludeRules()});
                        }
                    }
                }
                break;

            case Context::Rule::Type::RegExpr: {
                if (rule.isDotRegex) {
                    dotRegex.append(rule, nullptr);
                    break;
                }

                // check that `rule` does not have another RegExpr as a prefix
                RuleIterator ruleIterator(observedRules, observedRule);
                while (const auto *rulePtr = ruleIterator.next()) {
                    if (isUnreachable) {
                        break;
                    }
                    const auto &rule2 = *rulePtr;
                    if (rule2.type == Context::Rule::Type::RegExpr && isCompatible(rule2) && rule.insensitive == rule2.insensitive
                        && rule.dynamic == rule2.dynamic && rule.sanitizedString.startsWith(rule2.sanitizedString)) {
                        bool add = (rule.sanitizedString.startsWith(rule2.string) || rule.sanitizedString.size() < rule2.sanitizedString.size() + 2);
                        if (!add) {
                            // \s.* (sanitized = \s) is considered hiding \s*\S
                            // we check the quantifiers to see if this is the case
                            auto c1 = rule.sanitizedString[rule2.sanitizedString.size()].unicode();
                            auto c2 = rule.sanitizedString[rule2.sanitizedString.size() + 1].unicode();
                            auto c3 = rule2.sanitizedString.back().unicode();
                            if (c3 == '*' || c3 == '?' || c3 == '+') {
                                add = true;
                            } else if (c1 == '*' || c1 == '?') {
                                add = !((c2 == '?' || c2 == '+') || (rule.sanitizedString.size() >= rule2.sanitizedString.size() + 3));
                            } else {
                                add = true;
                            }
                        }
                        if (add) {
                            updateUnreachable1({&rule2, ruleIterator.currentIncludeRules()});
                        }
                    }
                }

                Q_FALLTHROUGH();
            }
            // check if a rule does not have another rule as a prefix
            case Context::Rule::Type::WordDetect:
            case Context::Rule::Type::StringDetect: {
                // check that dynamic `rule` does not have another dynamic StringDetect as a prefix
                if (rule.type == Context::Rule::Type::StringDetect && rule.dynamic == XmlBool::True) {
                    RuleIterator ruleIterator(observedRules, observedRule);
                    while (const auto *rulePtr = ruleIterator.next()) {
                        if (isUnreachable) {
                            break;
                        }

                        const auto &rule2 = *rulePtr;
                        if (rule2.type != Context::Rule::Type::StringDetect || rule2.dynamic != XmlBool::True || !isCompatible(rule2)) {
                            continue;
                        }

                        const bool isSensitive = (rule2.insensitive == XmlBool::True);
                        const auto caseSensitivity = isSensitive ? Qt::CaseInsensitive : Qt::CaseSensitive;
                        if ((isSensitive || rule.insensitive != XmlBool::True) && rule.string.startsWith(rule2.string, caseSensitivity)) {
                            updateUnreachable1({&rule2, ruleIterator.currentIncludeRules()});
                        }
                    }
                }

                // string used for comparison and truncated from "dynamic" part
                QStringView s = rule.string;

                // truncate to '%' with dynamic rules
                if (rule.dynamic == XmlBool::True) {
                    static const QRegularExpression dynamicPosition(QStringLiteral(R"(^(?:[^%]*|%(?![1-9]))*)"));
                    auto result = dynamicPosition.match(rule.string);
                    s = s.sliced(0, result.capturedLength());
                    // check if hidden by DetectChar/AnyChar
                    if (s.size() + 2 <= rule.string.size()) {
                        auto tables = CharTableArray(dynamicDetectChars, rule);
                        updateUnreachable1(tables.find(s.data()[s.size() + 2]));
                    }
                }

                QString sanitizedRegex;
                // truncate to special character with RegExpr.
                // If regexp contains '|', `s` becomes empty.
                if (rule.type == Context::Rule::Type::RegExpr) {
                    static const QRegularExpression regularChars(QStringLiteral(R"(^(?:[^.?*+^$[{(\\|]+|\\[-.?*+^$[\]{}()\\|]+|\[[^^\\]\])+)"));
                    static const QRegularExpression sanitizeChars(QStringLiteral(R"(\\([-.?*+^$[\]{}()\\|])|\[([^^\\])\])"));
                    const qsizetype result = regularChars.match(rule.string).capturedLength();
                    const qsizetype pos = qMin(result, s.size());
                    if (rule.string.indexOf(u'|', pos) < pos) {
                        sanitizedRegex = rule.string.sliced(0, qMin(result, s.size()));
                        sanitizedRegex.replace(sanitizeChars, QStringLiteral("\\1"));
                        s = sanitizedRegex;
                    } else {
                        s = QStringView();
                    }
                }

                // check if hidden by DetectChar/AnyChar
                if (s.size() > 0) {
                    auto t = CharTableArray(detectChars, rule);
                    if (rule.insensitive != XmlBool::True) {
                        updateUnreachable1(t.find(s[0]));
                    } else {
                        QChar c2[]{s[0].toLower(), s[0].toUpper()};
                        updateUnreachable2(t.find(QStringView(c2, 2)));
                    }

                    // StringDetect is a DetectChar
                    if (rule.type == Context::Rule::Type::StringDetect && rule.string.size() == 1) {
                        auto tables = CharTableArray(detectChars, rule);
                        auto c = rule.string[0];
                        if (rule.insensitive != XmlBool::True) {
                            c = c.toLower();
                            tables.removeNonSpecialWhenSpecial();
                            tables.append(c, rule);
                            c = c.toUpper();
                        }
                        tables.removeNonSpecialWhenSpecial();
                        tables.append(c, rule);
                    }
                }

                // check if Detect2Chars, StringDetect, WordDetect is not a prefix of s
                if (s.size() > 0 && !isUnreachable) {
                    // combination of uppercase and lowercase
                    RuleAndInclude detect2CharsInsensitives[]{{}, {}, {}, {}};

                    RuleIterator ruleIterator(observedRules, observedRule);
                    while (const auto *rulePtr = ruleIterator.next()) {
                        if (isUnreachable) {
                            break;
                        }
                        const auto &rule2 = *rulePtr;
                        const bool isSensitive = (rule2.insensitive == XmlBool::True);
                        const auto caseSensitivity = isSensitive ? Qt::CaseInsensitive : Qt::CaseSensitive;

                        switch (rule2.type) {
                        // check that it is not a detectChars prefix
                        case Context::Rule::Type::Detect2Chars:
                            if (isCompatible(rule2) && s.size() >= 2) {
                                if (rule.insensitive != XmlBool::True) {
                                    if (rule2.char0 == s[0] && rule2.char1 == s[1]) {
                                        updateUnreachable1({&rule2, ruleIterator.currentIncludeRules()});
                                    }
                                } else {
                                    // when the string is case insensitive,
                                    // all 4 upper/lower case combinations must be found
                                    auto set = [&](RuleAndInclude &x, QChar c1, QChar c2) {
                                        if (!x && rule2.char0 == c1 && rule2.char0 == c2) {
                                            x = {&rule2, ruleIterator.currentIncludeRules()};
                                        }
                                    };
                                    set(detect2CharsInsensitives[0], s[0].toLower(), s[1].toLower());
                                    set(detect2CharsInsensitives[1], s[0].toLower(), s[1].toUpper());
                                    set(detect2CharsInsensitives[2], s[0].toUpper(), s[1].toUpper());
                                    set(detect2CharsInsensitives[3], s[0].toUpper(), s[1].toLower());

                                    if (detect2CharsInsensitives[0] && detect2CharsInsensitives[1] && detect2CharsInsensitives[2]
                                        && detect2CharsInsensitives[3]) {
                                        isUnreachable = true;
                                        unreachableBy.append(detect2CharsInsensitives[0]);
                                        unreachableBy.append(detect2CharsInsensitives[1]);
                                        unreachableBy.append(detect2CharsInsensitives[2]);
                                        unreachableBy.append(detect2CharsInsensitives[3]);
                                    }
                                }
                            }
                            break;

                        // check that it is not a StringDetect prefix
                        case Context::Rule::Type::StringDetect:
                            if (isCompatible(rule2) && rule2.dynamic != XmlBool::True && (isSensitive || rule.insensitive != XmlBool::True)
                                && s.startsWith(rule2.string, caseSensitivity)) {
                                updateUnreachable1({&rule2, ruleIterator.currentIncludeRules()});
                            }
                            break;

                        // check if a WordDetect is hidden by another WordDetect
                        case Context::Rule::Type::WordDetect:
                            if (rule.type == Context::Rule::Type::WordDetect && isCompatible(rule2) && (isSensitive || rule.insensitive != XmlBool::True)
                                && 0 == rule.string.compare(rule2.string, caseSensitivity)) {
                                updateUnreachable1({&rule2, ruleIterator.currentIncludeRules()});
                            }
                            break;

                        default:;
                        }
                    }
                }

                break;
            }

            // check if hidden by another keyword rule
            case Context::Rule::Type::keyword: {
                RuleIterator ruleIterator(observedRules, observedRule);
                while (const auto *rulePtr = ruleIterator.next()) {
                    if (isUnreachable) {
                        break;
                    }
                    const auto &rule2 = *rulePtr;
                    if (rule2.type == Context::Rule::Type::keyword && isCompatible(rule2) && rule.string == rule2.string) {
                        updateUnreachable1({&rule2, ruleIterator.currentIncludeRules()});
                    }
                }
                // TODO check that all keywords are hidden by another rules
                break;
            }

            // add characters in those used but without checking if they are already.
            //  <DetectChar char="}" />
            //  <includedRules .../> <- reference an another <DetectChar char="}" /> who will not be checked
            //  <includedRules .../> <- reference a <DetectChar char="{" /> who will be added
            //  <DetectChar char="{" /> <- hidden by previous rule
            case Context::Rule::Type::IncludeRules:
                if (observedRule.includeRules && !observedRule.hasResolvedIncludeRules()) {
                    break;
                }

                if (auto &ruleAndInclude = includeContexts[rule.context.context]) {
                    updateUnreachable1(ruleAndInclude);
                } else {
                    ruleAndInclude.rule = &rule;
                }

                for (const auto *rulePtr : rule.includedIncludeRules) {
                    includeContexts.insert(rulePtr->context.context, RuleAndInclude{rulePtr, &rule});
                }

                if (observedRule.includeRules) {
                    break;
                }

                for (const auto *rulePtr : rule.includedRules) {
                    const auto &rule2 = *rulePtr;
                    switch (rule2.type) {
                    case Context::Rule::Type::AnyChar: {
                        auto tables = CharTableArray(detectChars, rule2);
                        tables.removeNonSpecialWhenSpecial();
                        tables.append(rule2.string, rule2, &rule);
                        break;
                    }

                    case Context::Rule::Type::DetectChar: {
                        auto &chars4 = (rule2.dynamic != XmlBool::True) ? detectChars : dynamicDetectChars;
                        auto tables = CharTableArray(chars4, rule2);
                        tables.removeNonSpecialWhenSpecial();
                        tables.append(rule2.char0, rule2, &rule);
                        break;
                    }

                    case Context::Rule::Type::DetectSpaces: {
                        auto tables = CharTableArray(detectChars, rule2);
                        tables.removeNonSpecialWhenSpecial();
                        tables.append(u' ', rule2, &rule);
                        tables.append(u'\t', rule2, &rule);
                        break;
                    }

                    case Context::Rule::Type::HlCChar:
                        hlCCharRule.setRule(rule2, &rule);
                        break;

                    case Context::Rule::Type::HlCHex:
                        hlCHexRule.setRule(rule2, &rule);
                        break;

                    case Context::Rule::Type::HlCOct:
                        hlCOctRule.setRule(rule2, &rule);
                        break;

                    case Context::Rule::Type::HlCStringChar:
                        hlCStringCharRule.setRule(rule2, &rule);
                        break;

                    case Context::Rule::Type::Int:
                        intRule.setRule(rule2, &rule);
                        break;

                    case Context::Rule::Type::Float:
                        floatRule.setRule(rule2, &rule);
                        break;

                    case Context::Rule::Type::LineContinue: {
                        auto tables = CharTableArray(lineContinueChars, rule2);
                        tables.removeNonSpecialWhenSpecial();
                        tables.append(rule2.char0, rule2, &rule);
                        break;
                    }

                    case Context::Rule::Type::RegExpr:
                        if (rule2.isDotRegex) {
                            dotRegex.append(rule2, &rule);
                        }
                        break;

                    case Context::Rule::Type::StringDetect: {
                        // StringDetect is a DetectChar
                        if (rule2.string.size() == 1 || (rule2.string.size() == 2 && rule2.dynamic == XmlBool::True)) {
                            auto &chars4 = (rule2.dynamic != XmlBool::True) ? detectChars : dynamicDetectChars;
                            auto tables = CharTableArray(chars4, rule2);
                            tables.removeNonSpecialWhenSpecial();
                            tables.append(rule2.string.back(), rule2, &rule);
                        }
                        break;
                    }

                    case Context::Rule::Type::WordDetect:
                    case Context::Rule::Type::Detect2Chars:
                    case Context::Rule::Type::IncludeRules:
                    case Context::Rule::Type::DetectIdentifier:
                    case Context::Rule::Type::keyword:
                    case Context::Rule::Type::Unknown:
                    case Context::Rule::Type::RangeDetect:
                        break;
                    }
                }
                break;

            case Context::Rule::Type::Unknown:
                break;
            }

            if (observedRule.includeRules && !observedRule.hasResolvedIncludeRules()) {
                auto &unreachableIncludedRule = unreachableIncludedRules[&rule];
                if (isUnreachable && unreachableIncludedRule.alwaysUnreachable) {
                    unreachableIncludedRule.unreachableBy.append(unreachableBy);
                } else {
                    unreachableIncludedRule.alwaysUnreachable = false;
                }
            } else if (isUnreachable) {
                success = false;
                QString message;
                message.reserve(128);
                for (auto &ruleAndInclude : std::as_const(unreachableBy)) {
                    message += u"line "_sv;
                    if (ruleAndInclude.includeRules) {
                        message += QString::number(ruleAndInclude.includeRules->line);
                        message += u" [by '"_sv;
                        message += ruleAndInclude.includeRules->context.name;
                        message += u"' line "_sv;
                        message += QString::number(ruleAndInclude.rule->line);
                        if (ruleAndInclude.includeRules->filename != ruleAndInclude.rule->filename) {
                            message += u" ("_sv;
                            message += ruleAndInclude.rule->filename;
                            message += u')';
                        }
                        message += u']';
                    } else {
                        message += QString::number(ruleAndInclude.rule->line);
                    }
                    message += u", "_sv;
                }
                message.chop(2);
                qWarning() << filename << "line" << rule.line << "unreachable rule by" << message;
            }
        }

        return success;
    }

    //! Proposes to merge certain rule sequences
    //! - several DetectChar/AnyChar into AnyChar
    //! - several RegExpr into one RegExpr
    bool suggestRuleMerger(const QString &filename, const Context &context) const
    {
        bool success = true;

        if (context.rules.isEmpty()) {
            return success;
        }

        auto it = context.rules.begin();
        const auto end = context.rules.end() - 1;

        for (; it < end; ++it) {
            const auto &rule1 = *it;
            const auto &rule2 = it[1];

            auto isCommonCompatible = [&] {
                if (rule1.lookAhead != rule2.lookAhead) {
                    return false;
                }
                // ignore attribute when lookAhead is true
                if (rule1.lookAhead != XmlBool::True && rule1.attribute != rule2.attribute) {
                    return false;
                }
                // clang-format off
                return rule1.beginRegion == rule2.beginRegion
                    && rule1.endRegion == rule2.endRegion
                    && rule1.firstNonSpace == rule2.firstNonSpace
                    && rule1.context.context == rule2.context.context
                    && rule1.context.popCount == rule2.context.popCount;
                // clang-format on
            };

            switch (rule1.type) {
            // request to merge StringDetect with AnyChar
            case Context::Rule::Type::StringDetect:
                if (rule1.string.size() != 1 || rule1.dynamic == XmlBool::True) {
                    break;
                }
                Q_FALLTHROUGH();
            // request to merge AnyChar/DetectChar
            case Context::Rule::Type::AnyChar:
            case Context::Rule::Type::DetectChar:
                if ((rule2.type == Context::Rule::Type::AnyChar || rule2.type == Context::Rule::Type::DetectChar
                     || (rule2.type == Context::Rule::Type::StringDetect && rule2.dynamic != XmlBool::True && rule2.string.size() == 1))
                    && isCommonCompatible() && rule1.column == rule2.column) {
                    qWarning() << filename << "line" << rule2.line << "can be merged as AnyChar with the previous rule";
                    success = false;
                }
                break;

            // request to merge multiple RegExpr
            case Context::Rule::Type::RegExpr:
                if (rule2.type == Context::Rule::Type::RegExpr && isCommonCompatible() && rule1.dynamic == rule2.dynamic
                    && (rule1.column == rule2.column || (rule1.column <= 0 && rule2.column <= 0))) {
                    qWarning() << filename << "line" << rule2.line << "can be merged with the previous rule";
                    success = false;
                }
                break;

            case Context::Rule::Type::DetectSpaces:
            case Context::Rule::Type::HlCChar:
            case Context::Rule::Type::HlCHex:
            case Context::Rule::Type::HlCOct:
            case Context::Rule::Type::HlCStringChar:
            case Context::Rule::Type::Int:
            case Context::Rule::Type::Float:
            case Context::Rule::Type::LineContinue:
            case Context::Rule::Type::WordDetect:
            case Context::Rule::Type::Detect2Chars:
            case Context::Rule::Type::IncludeRules:
            case Context::Rule::Type::DetectIdentifier:
            case Context::Rule::Type::keyword:
            case Context::Rule::Type::Unknown:
            case Context::Rule::Type::RangeDetect:
                break;
            }
        }

        return success;
    }

    //! Initialize the referenced context (ContextName::context)
    //! Some input / output examples are:
    //! - "#stay"         -> ""
    //! - "#pop"          -> ""
    //! - "Comment"       -> "Comment"
    //! - "#pop!Comment"  -> "Comment"
    //! - "##ISO C++"     -> ""
    //! - "Comment##ISO C++"-> "Comment" in ISO C++
    void resolveContextName(Definition &definition, Context &context, ContextName &contextName, int line)
    {
        QStringView name = contextName.name;
        if (name.isEmpty()) {
            contextName.stay = true;
        } else if (name.startsWith(u"#stay"_sv)) {
            contextName.stay = true;
            if (name.size() > 5) {
                qWarning() << definition.filename << "line" << line << "invalid context in" << context.name;
                m_success = false;
            }
        } else {
            while (name.startsWith(u"#pop"_sv)) {
                name = name.sliced(4);
                ++contextName.popCount;
            }

            if (contextName.popCount && !name.isEmpty()) {
                if (name.startsWith(u'!') && name.size() > 1) {
                    name = name.sliced(1);
                } else {
                    qWarning() << definition.filename << "line" << line << "'!' missing between '#pop' and context name" << context.name;
                    m_success = false;
                }
            }

            if (!name.isEmpty()) {
                const int idx = name.indexOf(u"##"_sv);
                if (idx == -1) {
                    auto it = definition.contexts.find(name.toString());
                    if (it != definition.contexts.end()) {
                        contextName.context = &*it;
                    }
                } else {
                    auto defName = name.sliced(idx + 2);
                    auto it = m_definitions.find(defName.toString());
                    if (it != m_definitions.end()) {
                        auto listName = name.sliced(0, idx).toString();
                        definition.referencedDefinitions.insert(&*it);
                        auto ctxIt = it->contexts.find(listName.isEmpty() ? it->firstContextName : listName);
                        if (ctxIt != it->contexts.end()) {
                            contextName.context = &*ctxIt;
                        }
                    } else {
                        qWarning() << definition.filename << "line" << line << "unknown definition in" << context.name;
                        m_success = false;
                    }
                }

                if (!contextName.context) {
                    qWarning() << definition.filename << "line" << line << "unknown context" << name << "in" << context.name;
                    m_success = false;
                }
            }
        }
    }

    QMap<QString, Definition> m_definitions;
    QHash<QString, QString> m_names;
    Definition *m_currentDefinition = nullptr;
    Keywords *m_currentKeywords = nullptr;
    Context *m_currentContext = nullptr;
    // xml reader variable
    //@{
    QString m_textContent;
    bool m_inKeywordItem = false;
    //@}
    bool m_success = true;
};

class HlCompressor
{
public:
    HlCompressor(const QString &kateVersion)
        : m_kateVersion(kateVersion)
    {
        m_hasElems.push_back(true);
    }

    const QString &compressedXML() const
    {
        return m_data;
    }

    /**
     * Reduce xml space by removing what is superfluous.
     * - transforms boolean values into 0 or 1.
     * - remove unused attributes.
     * - remove spaces and comments.
     * - remove context attributes referring to #stay (because this is the default).
     * - replace Detect2Chars with StringDetect (String="xy" is shorter than char="x" char1="y").
     * - sort contexts by frequency of use to accelerate their search during loading.
     */
    void processElement(const QXmlStreamReader &xml)
    {
        switch (xml.tokenType()) {
        case QXmlStreamReader::StartElement: {
            closePreviousOpenTag(m_inContexts && !m_contexts.empty() ? m_contexts.back().data : m_data);
            m_hasElems.push_back(false);

            const auto tagName = xml.name();
            if (tagName == u"contexts"_sv) {
                m_inContexts = true;
                m_data += u"<contexts"_sv;
            } else if (m_inContexts) {
                Context &ctx = (m_contexts.empty() || tagName == u"context"_sv) ? m_contexts.emplace_back() : m_contexts.back();
                QString &out = ctx.data;
                const bool isDetect2Chars = tagName == u"Detect2Chars"_sv;
                out += u'<' % (isDetect2Chars ? u"StringDetect"_sv : tagName);

                auto attrs = xml.attributes();
                sortAttributes(attrs);
                for (const auto &attr : attrs) {
                    const auto attrName = attr.name();
                    auto value = attr.value();
                    // transform Detect2Chars char and char1 attributes to StringDetect String attribute
                    if (isDetect2Chars && (attrName == u"char"_sv || attrName == u"char1"_sv)) {
                        if (attrName == u"char"_sv) {
                            const auto ch0 = value;
                            const auto ch1 = attrs.value(u"char1"_sv);
                            QChar chars[]{ch0.isEmpty() ? u' ' : ch0[0], ch1.isEmpty() ? u' ' : ch1[0]};
                            writeXmlAttribute(out, u"String"_sv, QStringView(chars, 2), tagName);
                        }
                    } else if (attrName == u"context"_sv || attrName == u"lineEndContext"_sv || attrName == u"fallthroughContext"_sv
                               || attrName == u"lineEmptyContext"_sv) {
                        // ignore #stay context because this is the default
                        if (value != u"#stay"_sv) {
                            writeXmlAttribute(out, attrName, value, tagName);

                            /*
                             * Extract context name and increment context counter
                             */
                            bool hasPop = false;
                            while (value.startsWith(u"#pop"_sv)) {
                                hasPop = true;
                                value = value.sliced(4);
                            }
                            if (hasPop && !value.isEmpty()) {
                                value = value.sliced(1);
                            }
                            if (!value.isEmpty() && -1 == value.indexOf(u"##"_sv)) {
                                m_contextRefs[value.toString()]++;
                            }
                        }
                    } else if (tagName == u"LineContinue"_sv && attrName == u"char"_sv && value == u"\\") {
                        // ignore char="\\" with LineContinue
                    } else {
                        if (attrName == u"name"_sv) {
                            ctx.name = value.toString();
                        }
                        writeXmlAttribute(out, attrName, value, tagName);
                    }
                }
            } else {
                m_data += u'<' % tagName;
                const auto attrs = xml.attributes();
                for (const auto &attr : attrs) {
                    auto name = attr.name();
                    auto value = (name == u"kateversion") ? QStringView(m_kateVersion) : attr.value();
                    writeXmlAttribute(m_data, name, value, tagName);
                }
            }
            break;
        }

        case QXmlStreamReader::EndElement: {
            const auto name = xml.name();
            if (m_inContexts && !m_contexts.empty() && name == u"contexts"_sv) {
                m_inContexts = false;
                // sorting contexts by the most used (ignore first context)
                std::sort(m_contexts.begin() + 1, m_contexts.end(), [&](auto &ctx1, auto &ctx2) {
                    auto i1 = m_contextRefs.value(ctx1.name);
                    auto i2 = m_contextRefs.value(ctx2.name);
                    if (i1 != i2) {
                        return i1 > i2;
                    }
                    // for a reproducible build, contexts with the same number of uses are sorted by name
                    return ctx1.name < ctx2.name;
                });
                for (const auto &ctx : m_contexts) {
                    m_data += ctx.data;
                }
            }

            QString &out = m_inContexts && !m_contexts.empty() ? m_contexts.back().data : m_data;
            if (m_hasElems.back()) {
                out += u"</"_sv % name % u'>';
            } else {
                out += u"/>"_sv;
            }
            m_hasElems.pop_back();
            break;
        }

        case QXmlStreamReader::EntityReference:
        case QXmlStreamReader::Characters:
            if (!m_inContexts && !xml.isWhitespace()) {
                closePreviousOpenTag(m_data);
                writeXmlText(m_data, xml.text());
            }
            break;

        default:;
        }
    }

private:
    void closePreviousOpenTag(QString &out)
    {
        if (!m_hasElems.back()) {
            m_hasElems.back() = true;
            out += u'>';
        }
    }

    /**
     * Write \p text escaping special characters.
     */
    static void writeXmlText(QString &out, QStringView text, bool escapeDQ = false)
    {
        for (const QChar &c : text) {
            if (c == u'<') {
                out += u"&lt;"_sv;
            } else if (c == u'&') {
                out += u"&amp;"_sv;
            } else if (escapeDQ && c == u'"') {
                out += u"&#34;"_sv;
            } else if (c == u'\t') {
                // non-space whitespace character in an attribute is remplaced with space...
                out += u"&#9;"_sv;
            } else {
                out += c;
            }
        }
    }

    /**
     * Write attribut in \p out.
     * Booleans are converted to 0, 1 or ignored if this corresponds to the default value.
     * Values will be written with either double quotes or single quotes,
     * depending on which takes up the least space
     */
    static void writeXmlAttribute(QString &out, QStringView attrName, QStringView value, QStringView tagName)
    {
        enum class DefaultBool {
            // default value is false
            False,
            // default value is true
            True,
            // manipulate as a tribool whose attribute absence is equivalent to None
            None,
            // not used
            Ignored,
            // default value is false, but None for <keyword>
            FalseOrKeywordTag,
            // default value is true, but depends on another value for <keywords>
            TrueOrKeywordsTag,
            // default is false, but ignored in <context>
            DynamicAttr,
        };
        static const QHash<QStringView, DefaultBool> booleanAttrs({
            {u"fallthrough"_sv, DefaultBool::Ignored},
            {u"dynamic"_sv, DefaultBool::DynamicAttr},
            {u"hidden"_sv, DefaultBool::False},
            {u"indentationsensitive"_sv, DefaultBool::False},
            {u"noIndentationBasedFolding"_sv, DefaultBool::False},
            {u"lookAhead"_sv, DefaultBool::False},
            {u"firstNonSpace"_sv, DefaultBool::False},
            {u"insensitive"_sv, DefaultBool::FalseOrKeywordTag},
            {u"minimal"_sv, DefaultBool::False},
            {u"includeAttrib"_sv, DefaultBool::False},
            {u"italic"_sv, DefaultBool::None},
            {u"bold"_sv, DefaultBool::None},
            {u"underline"_sv, DefaultBool::None},
            {u"strikeOut"_sv, DefaultBool::None},
            {u"spellChecking"_sv, DefaultBool::True},
            {u"casesensitive"_sv, DefaultBool::TrueOrKeywordsTag},
            {u"ignored"_sv, DefaultBool::Ignored},
        });

        auto it = booleanAttrs.find(attrName);
        // convert boolean value
        if (it != booleanAttrs.end()) {
            bool b = KSyntaxHighlighting::Xml::attrToBool(value);
            bool ignoreAttr = false;
            switch (*it) {
            case DefaultBool::Ignored:
                ignoreAttr = true;
                break;
            case DefaultBool::TrueOrKeywordsTag:
                ignoreAttr = (tagName == u"keywords"_sv) ? false : b;
                break;
            case DefaultBool::True:
                ignoreAttr = b;
                break;
            case DefaultBool::FalseOrKeywordTag:
                ignoreAttr = (tagName == u"keyword"_sv) ? false : !b;
                break;
            case DefaultBool::DynamicAttr:
                ignoreAttr = (tagName == u"context"_sv) || !b;
                break;
            case DefaultBool::False:
                ignoreAttr = !b;
                break;
            case DefaultBool::None:
                ignoreAttr = false;
                break;
            }
            if (!ignoreAttr) {
                out += u' ' % attrName % u"=\""_sv % (b ? u'1' : u'0') % u'"';
            }
        } else {
            const bool hasDQ = value.contains(u'"');
            // attribute in double quotes when the value does not contain " or contains " and '
            if (!hasDQ || value.contains(u'\'')) {
                out += u' ' % attrName % u"=\""_sv;
                writeXmlText(out, value, hasDQ);
                out += u'"';
                // attribute in single quotes because the value contains "
            } else {
                out += u' ' % attrName % u"='"_sv;
                writeXmlText(out, value);
                out += u'\'';
            }
        }
    }

    /**
     * Sort attributes for better compression by rcc.
     */
    static void sortAttributes(QXmlStreamAttributes &attrs)
    {
        static const QHash<QStringView, int> priorityAttrs({
            // context and rule
            {u"attribute"_sv, 5},

            // context and itemData
            {u"name"_sv, 4},

            // context
            {u"noIndentationBasedFolding"_sv, 11},
            {u"lineEndContext"_sv, 9},
            {u"lineEmptyContext"_sv, 8},
            {u"fallthroughContext"_sv, 7},

            // rule
            {u"lookAhead"_sv, 100},
            {u"firstNonSpace"_sv, 99},
            {u"dynamic"_sv, 98},
            {u"minimal"_sv, 97},
            {u"includeAttrib"_sv, 96},
            {u"insensitive"_sv, 95},
            {u"column"_sv, 50},
            {u"beginRegion"_sv, 40},
            {u"endRegion"_sv, 41},
            {u"weakDeliminator"_sv, 31},
            {u"additionalDeliminator"_sv, 30},
            {u"context"_sv, 20},
            {u"String"_sv, 2},
            {u"char"_sv, 2},

            // itemData
            {u"strikeOut"_sv, 100},
            {u"underline"_sv, 99},
            {u"italic"_sv, 98},
            {u"bold"_sv, 97},
            {u"spellChecking"_sv, 96},
            {u"defStyleNum"_sv, 95},
            {u"color"_sv, 94},
            {u"backgroundColor"_sv, 93},
            {u"selBackgroundColor"_sv, 92},
            {u"selColor"_sv, 91},
        });
        std::sort(attrs.begin(), attrs.end(), [](auto &attr1, auto &attr2) {
            auto i1 = priorityAttrs.value(attr1.name());
            auto i2 = priorityAttrs.value(attr2.name());
            if (i1 != i2) {
                return i1 < i2;
            }
            return attr1.name() < attr2.name();
        });
    }

    struct Context {
        QString name;
        QString data;
    };
    QString m_data = u"<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE language>"_s;
    std::vector<Context> m_contexts;
    QHash<QString, int> m_contextRefs;
    QVarLengthArray<bool, 8> m_hasElems;
    QString m_kateVersion;
    bool m_inContexts = false;
};

void printFileError(const QFile &file)
{
    qWarning() << "Failed to open" << file.fileName() << "-" << file.errorString();
}

void printXmlError(const QString &fileName, const QXmlStreamReader &xml)
{
    qWarning() << fileName << "-" << xml.errorString() << "@ offset" << xml.characterOffset();
};

QStringList readListing(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        printFileError(file);
        return QStringList();
    }

    QXmlStreamReader xml(&file);
    QStringList listing;
    while (!xml.atEnd()) {
        xml.readNext();

        // add only .xml files, no .json or stuff
        if (xml.isCharacters() && xml.text().contains(QLatin1String(".xml"))) {
            listing.append(xml.text().toString());
        }
    }

    if (xml.hasError()) {
        printXmlError(fileName, xml);
        listing.clear();
    }

    return listing;
}

/**
 * check if the "extensions" attribute have valid wildcards
 * @param extensions extensions string to check
 * @return valid?
 */
bool checkExtensions(QStringView extensions)
{
    // get list of extensions
    const QList<QStringView> extensionParts = extensions.split(u';', Qt::SkipEmptyParts);

    // ok if empty
    if (extensionParts.isEmpty()) {
        return true;
    }

    // check that only valid wildcard things are inside the parts
    for (const auto &extension : extensionParts) {
        for (const auto c : extension) {
            // eat normal things
            if (c.isDigit() || c.isLetter()) {
                continue;
            }

            // allow some special characters
            if (c == u'.' || c == u'-' || c == u'_' || c == u'+') {
                continue;
            }

            // only allowed wildcard things: '?' and '*'
            if (c == u'?' || c == u'*') {
                continue;
            }

            qWarning() << "invalid character" << c << "seen in extensions wildcard";
            return false;
        }
    }

    // all checks passed
    return true;
}

struct CompressedFile {
    QString fileName;
    QString xmlData;
};

}

int main(int argc, char *argv[])
{
    // get app instance
    QCoreApplication app(argc, argv);

    // ensure enough arguments are passed
    if (app.arguments().size() < 4) {
        return 1;
    }

#ifdef HAS_XERCESC
    // care for proper init and cleanup
    XMLPlatformUtils::Initialize();
    auto cleanup = qScopeGuard(XMLPlatformUtils::Terminate);

    /*
     * parse XSD first time and cache it
     */
    XMLGrammarPoolImpl xsd(XMLPlatformUtils::fgMemoryManager);

    // create parser for the XSD
    CustomXMLValidator parser(&xsd);

    // load grammar into the pool, on error just abort
    const auto xsdFile = app.arguments().at(2);
    if (!parser.loadGrammar((const char16_t *)xsdFile.utf16(), Grammar::SchemaGrammarType, true) || parser.eh.failed()) {
        qWarning("Failed to parse XSD %s: %s", qPrintable(xsdFile), qPrintable(parser.messages));
        return 2;
    }

    // lock the pool, no later modifications wanted!
    xsd.lockPool();
#endif

    const QString hlFilenamesListing = app.arguments().value(3);
    if (hlFilenamesListing.isEmpty()) {
        return 1;
    }

    QStringList hlFilenames = readListing(hlFilenamesListing);
    if (hlFilenames.isEmpty()) {
        qWarning("Failed to read %s", qPrintable(hlFilenamesListing));
        return 3;
    }

    // text attributes
    const QStringList textAttributes = QStringList() << QStringLiteral("name") << QStringLiteral("alternativeNames") << QStringLiteral("section")
                                                     << QStringLiteral("mimetype") << QStringLiteral("extensions") << QStringLiteral("style")
                                                     << QStringLiteral("author") << QStringLiteral("license") << QStringLiteral("indenter");

    // index all given highlightings
    HlFilesChecker filesChecker;
    QVariantMap hls;
    int anyError = 0;
    std::vector<CompressedFile> compressedFiles;
    for (const QString &hlFilename : std::as_const(hlFilenames)) {
        QFile hlFile(hlFilename);
        if (!hlFile.open(QIODevice::ReadOnly)) {
            printFileError(hlFile);
            anyError = 3;
            continue;
        }

#ifdef HAS_XERCESC
        // create parser
        CustomXMLValidator parser(&xsd);

        // parse the XML file
        parser.parse((const char16_t *)hlFile.fileName().utf16());

        // report issues
        if (parser.eh.failed()) {
            qWarning("Failed to validate XML %s: %s", qPrintable(hlFile.fileName()), qPrintable(parser.messages));
            anyError = 4;
            continue;
        }
#endif

        // read the needed attributes from toplevel language tag
        hlFile.reset();
        QXmlStreamReader xml(&hlFile);
        if (xml.readNextStartElement()) {
            if (xml.name() != QLatin1String("language")) {
                anyError = 5;
                continue;
            }
        } else {
            anyError = 6;
            continue;
        }

        // map to store hl info
        QVariantMap hl;

        // transfer text attributes
        for (const QString &attribute : std::as_const(textAttributes)) {
            hl[attribute] = xml.attributes().value(attribute).toString();
        }

        // check if extensions have the right format
        if (!checkExtensions(hl[QStringLiteral("extensions")].toString())) {
            qWarning() << hlFilename << "'extensions' wildcards invalid:" << hl[QStringLiteral("extensions")].toString();
            anyError = 23;
        }

        // numerical attributes
        hl[QStringLiteral("version")] = xml.attributes().value(QLatin1String("version")).toInt();
        hl[QStringLiteral("priority")] = xml.attributes().value(QLatin1String("priority")).toInt();

        // add boolean one
        hl[QStringLiteral("hidden")] = attrToBool(xml.attributes().value(QLatin1String("hidden")));

        // keep some strings as UTF-8 for faster translations
        hl[QStringLiteral("nameUtf8")] = hl[QStringLiteral("name")].toString().toUtf8();
        hl[QStringLiteral("sectionUtf8")] = hl[QStringLiteral("section")].toString().toUtf8();

        // remember hl
        hls[QFileInfo(hlFile).fileName()] = hl;

        const QStringView kateversion = xml.attributes().value(QStringLiteral("kateversion"));
        const QString hlName = hl[QStringLiteral("name")].toString();
        const QString hlAlternativeNames = hl[QStringLiteral("alternativeNames")].toString();

        filesChecker.setDefinition(kateversion, hlFilename, hlName, hlAlternativeNames.split(u';', Qt::SkipEmptyParts));

        // As the compressor removes "fallthrough" attribute which is required with
        // "fallthroughContext" before the 5.62 version, the minimum version is
        // automatically increased
        HlCompressor compressor((filesChecker.currentVersion() < KateVersion{5, 62}) ? u"5.62"_s : kateversion.toString());
        compressor.processElement(xml);

        // scan for broken regex or keywords with spaces
        while (!xml.atEnd()) {
            xml.readNext();
            filesChecker.processElement(xml);
            compressor.processElement(xml);
        }

        if (xml.hasError()) {
            anyError = 33;
            printXmlError(hlFilename, xml);
        }

        compressedFiles.emplace_back(CompressedFile{
            QFileInfo(hlFilename).fileName(),
            compressor.compressedXML(),
        });
    }

    filesChecker.resolveContexts();

    if (!filesChecker.check()) {
        anyError = 7;
    }

    // bail out if any problem was seen
    if (anyError) {
        return anyError;
    }

    // check compressed file
    HlFilesChecker filesChecker2;
    const QString compressedDir = app.arguments().at(4) + u"/"_sv;
    for (const auto &compressedFile : std::as_const(compressedFiles)) {
        const auto outFileName = compressedDir + compressedFile.fileName;
        auto utf8Data = compressedFile.xmlData.toUtf8();

#ifdef HAS_XERCESC
        // create parser
        CustomXMLValidator parser(&xsd);

        auto utf8Filename = outFileName.toUtf8();
        utf8Filename.append('\0');
        // parse the XML file
        MemBufInputSource membuf(reinterpret_cast<const XMLByte *>(utf8Data.constData()), utf8Data.size(), utf8Filename.data());

        // report issues
        if (parser.eh.failed()) {
            qWarning("Failed to validate XML %s: %s", qPrintable(outFileName), qPrintable(parser.messages));
            return 8;
        }
#endif

        QBuffer buffer(&utf8Data);
        buffer.open(QBuffer::ReadOnly);
        QXmlStreamReader xml(&buffer);
        // scan for broken file
        while (!xml.atEnd()) {
            if (xml.readNext() == QXmlStreamReader::TokenType::StartElement && xml.name() == u"language"_sv) {
                const auto attrs = xml.attributes();
                const auto version = attrs.value(u"kateversion"_sv);
                const QString hlName = attrs.value(u"name"_sv).toString();
                const QString hlAlternativeNames = attrs.value(u"alternativeNames"_sv).toString();
                filesChecker2.setDefinition(version, outFileName, hlName, hlAlternativeNames.split(u';', Qt::SkipEmptyParts));
            }
            filesChecker2.processElement(xml);
        }

        if (xml.hasError()) {
            printXmlError(outFileName, xml);
            return 9;
        }

        // create outfile, after all has worked!
        QFile outFile(outFileName);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return 10;
        }
        outFile.write(utf8Data);
    }

    filesChecker2.resolveContexts();

    // bail out if any problem was seen
    if (!filesChecker2.check()) {
        return 11;
    }

    // create outfile, after all has worked!
    QFile outFile(app.arguments().at(1));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return 12;
    }

    // write out json
    outFile.write(QCborValue::fromVariant(QVariant(hls)).toCbor());

    // be done
    return 0;
}
