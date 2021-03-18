/*
    SPDX-FileCopyrightText: 2014 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include <QCborValue>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutableMapIterator>
#include <QRegularExpression>
#include <QVariant>
#include <QXmlStreamReader>

#ifdef QT_XMLPATTERNS_LIB
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#endif

#include "../lib/worddelimiters_p.h"
#include "../lib/xml_p.h"

#include <array>

using KSyntaxHighlighting::WordDelimiters;
using KSyntaxHighlighting::Xml::attrToBool;

class HlFilesChecker
{
public:
    void setDefinition(const QStringRef &verStr, const QString &filename, const QString &name)
    {
        m_currentDefinition = &*m_definitions.insert(name, Definition{});
        m_currentDefinition->languageName = name;
        m_currentDefinition->filename = filename;
        m_currentDefinition->kateVersionStr = verStr.toString();
        m_currentKeywords = nullptr;
        m_currentContext = nullptr;

        const auto idx = verStr.indexOf(QLatin1Char('.'));
        if (idx <= 0) {
            qWarning() << filename << "invalid kateversion" << verStr;
            m_success = false;
        } else {
            m_currentDefinition->kateVersion = {verStr.left(idx).toInt(), verStr.mid(idx + 1).toInt()};
        }
    }

    void processElement(QXmlStreamReader &xml)
    {
        if (xml.isStartElement()) {
            if (m_currentContext) {
                m_currentContext->rules.append(Context::Rule{});
                auto &rule = m_currentContext->rules.back();
                m_success = rule.parseElement(m_currentDefinition->filename, xml) && m_success;
            } else if (m_currentKeywords) {
                m_success = m_currentKeywords->items.parseElement(m_currentDefinition->filename, xml) && m_success;
            } else if (xml.name() == QStringLiteral("context")) {
                processContextElement(xml);
            } else if (xml.name() == QStringLiteral("list")) {
                processListElement(xml);
            } else if (xml.name() == QStringLiteral("keywords")) {
                m_success = m_currentDefinition->parseKeywords(xml) && m_success;
            } else if (xml.name() == QStringLiteral("emptyLine")) {
                m_success = parseEmptyLine(m_currentDefinition->filename, xml) && m_success;
            } else if (xml.name() == QStringLiteral("itemData")) {
                m_success = m_currentDefinition->itemDatas.parseElement(m_currentDefinition->filename, xml) && m_success;
            }
        } else if (xml.isEndElement()) {
            if (m_currentContext && xml.name() == QStringLiteral("context")) {
                m_currentContext = nullptr;
            } else if (m_currentKeywords && xml.name() == QStringLiteral("list")) {
                m_currentKeywords = nullptr;
            }
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

            QMutableMapIterator<QString, Context> contextIt(contexts);
            while (contextIt.hasNext()) {
                contextIt.next();
                auto &context = contextIt.value();
                resolveContextName(definition, context, context.lineEndContext, context.line);
                resolveContextName(definition, context, context.lineEmptyContext, context.line);
                resolveContextName(definition, context, context.fallthroughContext, context.line);
                for (auto &rule : context.rules) {
                    resolveContextName(definition, context, rule.context, rule.line);
                }
            }

            definition.firstContext = &*definition.contexts.find(definition.firstContextName);
        }

        resolveIncludeRules();
    }

    bool check() const
    {
        bool success = m_success;

        const auto usedContexts = extractUsedContexts();

        QMap<const Definition *, const Definition *> maxVersionByDefinitions;

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

            QSet<const Keywords *> referencedKeywords;
            QSet<ItemDatas::Style> usedAttributeNames;
            success = checkKeywordsList(definition, referencedKeywords) && success;
            success = checkContexts(definition, referencedKeywords, usedAttributeNames, usedContexts) && success;

            // search for non-existing or unreferenced keyword lists.
            for (const auto &keywords : definition.keywordsList) {
                if (!referencedKeywords.contains(&keywords)) {
                    qWarning() << filename << "line" << keywords.line << "unused keyword:" << keywords.name;
                }
            }

            // search for non-existing itemDatas.
            const auto invalidNames = usedAttributeNames - definition.itemDatas.styleNames;
            for (const auto &styleName : invalidNames) {
                qWarning() << filename << "line" << styleName.line << "reference of non-existing itemData attributes:" << styleName.name;
                success = false;
            }

            // search for unused itemDatas.
            const auto unusedNames = definition.itemDatas.styleNames - usedAttributeNames;
            for (const auto &styleName : unusedNames) {
                qWarning() << filename << "line" << styleName.line << "unused itemData:" << styleName.name;
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

        const Context *context = nullptr;
    };

    struct Parser {
        const QString &filename;
        QXmlStreamReader &xml;
        QXmlStreamAttribute &attr;
        bool success;

        //! Read a string type attribute, \c sucess = \c false when \p str is not empty
        //! \return \c true when attr.name() == attrName, otherwise false
        bool extractString(QString &str, const QString &attrName)
        {
            if (attr.name() != attrName)
                return false;

            str = attr.value().toString();
            if (str.isEmpty()) {
                qWarning() << filename << "line" << xml.lineNumber() << attrName << "attribute is empty";
                success = false;
            }

            return true;
        }

        //! Read a bool type attribute, \c sucess = \c false when \p xmlBool is not \c XmlBool::Unspecified.
        //! \return \c true when attr.name() == attrName, otherwise false
        bool extractXmlBool(XmlBool &xmlBool, const QString &attrName)
        {
            if (attr.name() != attrName)
                return false;

            xmlBool = attr.value().isNull() ? XmlBool::Unspecified : attrToBool(attr.value()) ? XmlBool::True : XmlBool::False;

            return true;
        }

        //! Read a positive integer type attribute, \c sucess = \c false when \p positive is already greater than or equal to 0
        //! \return \c true when attr.name() == attrName, otherwise false
        bool extractPositive(int &positive, const QString &attrName)
        {
            if (attr.name() != attrName)
                return false;

            bool ok = true;
            positive = attr.value().toInt(&ok);

            if (!ok || positive < 0) {
                qWarning() << filename << "line" << xml.lineNumber() << attrName << "should be a positive integer:" << attr.value();
                success = false;
            }

            return true;
        }

        //! Read a color, \c sucess = \c false when \p color is already greater than or equal to 0
        //! \return \c true when attr.name() == attrName, otherwise false
        bool checkColor(const QString &attrName)
        {
            if (attr.name() != attrName)
                return false;

            const auto value = attr.value().toString();
            if (value.isEmpty() /*|| QColor(value).isValid()*/) {
                qWarning() << filename << "line" << xml.lineNumber() << attrName << "should be a color:" << attr.value();
                success = false;
            }

            return true;
        }

        //! Read a QChar, \c sucess = \c false when \p c is not \c '\0' or does not have one char
        //! \return \c true when attr.name() == attrName, otherwise false
        bool extractChar(QChar &c, const QString &attrName)
        {
            if (attr.name() != attrName)
                return false;

            if (attr.value().size() == 1)
                c = attr.value()[0];
            else {
                c = QLatin1Char('_');
                qWarning() << filename << "line" << xml.lineNumber() << attrName << "must contain exactly one char:" << attr.value();
                success = false;
            }

            return true;
        }

        //! \return parsing status when \p isExtracted is \c true, otherwise \c false
        bool checkIfExtracted(bool isExtracted)
        {
            if (isExtracted)
                return success;

            qWarning() << filename << "line" << xml.lineNumber() << "unknown attribute:" << attr.name();
            return false;
        }
    };

    struct Keywords {
        struct Items {
            struct Item {
                QString content;
                int line;

                friend uint qHash(const Item &item, uint seed = 0)
                {
                    return qHash(item.content, seed);
                }

                friend bool operator==(const Item &item0, const Item &item1)
                {
                    return item0.content == item1.content;
                }
            };

            QVector<Item> keywords;
            QSet<Item> includes;

            bool parseElement(const QString &filename, QXmlStreamReader &xml)
            {
                bool success = true;

                const int line = xml.lineNumber();
                QString content = xml.readElementText();

                if (content.isEmpty()) {
                    qWarning() << filename << "line" << line << "is empty:" << xml.name();
                    success = false;
                }

                if (xml.name() == QStringLiteral("include")) {
                    includes.insert({content, line});
                } else if (xml.name() == QStringLiteral("item")) {
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

        bool parseElement(const QString &filename, QXmlStreamReader &xml)
        {
            line = xml.lineNumber();

            bool success = true;
            for (auto &attr : xml.attributes()) {
                Parser parser{filename, xml, attr, success};

                const bool isExtracted = parser.extractString(name, QStringLiteral("name"));

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

            // DetectChar, Detect2Chars, LineContinue, RangeDetect
            QChar char0;
            // Detect2Chars, RangeDetect
            QChar char1;

            // AnyChar, DetectChar, StringDetect, RegExpr, WordDetect, keyword
            QString string;

            // Float, HlCHex, HlCOct, Int, WordDetect, keyword
            QString additionalDeliminator;
            QString weakDeliminator;

            // rules included by IncludeRules
            QVector<const Rule *> includedRules;

            // IncludeRules included by IncludeRules
            QSet<const Rule *> includedIncludeRules;

            QString filename;

            bool parseElement(const QString &filename, QXmlStreamReader &xml)
            {
                this->filename = filename;

                line = xml.lineNumber();

                using Pair = QPair<QString, Type>;
                static const auto pairs = {
                    Pair{QStringLiteral("AnyChar"), Type::AnyChar},
                    Pair{QStringLiteral("Detect2Chars"), Type::Detect2Chars},
                    Pair{QStringLiteral("DetectChar"), Type::DetectChar},
                    Pair{QStringLiteral("DetectIdentifier"), Type::DetectIdentifier},
                    Pair{QStringLiteral("DetectSpaces"), Type::DetectSpaces},
                    Pair{QStringLiteral("Float"), Type::Float},
                    Pair{QStringLiteral("HlCChar"), Type::HlCChar},
                    Pair{QStringLiteral("HlCHex"), Type::HlCHex},
                    Pair{QStringLiteral("HlCOct"), Type::HlCOct},
                    Pair{QStringLiteral("HlCStringChar"), Type::HlCStringChar},
                    Pair{QStringLiteral("IncludeRules"), Type::IncludeRules},
                    Pair{QStringLiteral("Int"), Type::Int},
                    Pair{QStringLiteral("LineContinue"), Type::LineContinue},
                    Pair{QStringLiteral("RangeDetect"), Type::RangeDetect},
                    Pair{QStringLiteral("RegExpr"), Type::RegExpr},
                    Pair{QStringLiteral("StringDetect"), Type::StringDetect},
                    Pair{QStringLiteral("WordDetect"), Type::WordDetect},
                    Pair{QStringLiteral("keyword"), Type::keyword},
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
                            // remove parentheses on a double from the string
                            auto reg = QString(string).replace(removeParentheses, QString());
                            isDotRegex = reg.contains(isDot);
                        }
                        return success;
                    }
                }

                qWarning() << filename << "line" << xml.lineNumber() << "unknown element:" << xml.name();
                return false;
            }

        private:
            bool parseAttributes(const QString &filename, QXmlStreamReader &xml)
            {
                bool success = true;

                for (auto &attr : xml.attributes()) {
                    Parser parser{filename, xml, attr, success};
                    XmlBool includeAttrib{};

                    // clang-format off
                    const bool isExtracted
                        = parser.extractString(attribute, QStringLiteral("attribute"))
                       || parser.extractString(context.name, QStringLiteral("context"))
                       || parser.extractXmlBool(lookAhead, QStringLiteral("lookAhead"))
                       || parser.extractXmlBool(firstNonSpace, QStringLiteral("firstNonSpace"))
                       || parser.extractString(beginRegion, QStringLiteral("beginRegion"))
                       || parser.extractString(endRegion, QStringLiteral("endRegion"))
                       || parser.extractPositive(column, QStringLiteral("column"))
                       || ((type == Type::RegExpr
                         || type == Type::StringDetect
                         || type == Type::WordDetect
                         || type == Type::keyword
                         ) && parser.extractXmlBool(insensitive, QStringLiteral("insensitive")))
                       || ((type == Type::DetectChar
                         || type == Type::RegExpr
                         || type == Type::StringDetect
                         || type == Type::keyword
                         ) && parser.extractXmlBool(dynamic, QStringLiteral("dynamic")))
                       || ((type == Type::RegExpr)
                           && parser.extractXmlBool(minimal, QStringLiteral("minimal")))
                       || ((type == Type::DetectChar
                         || type == Type::Detect2Chars
                         || type == Type::LineContinue
                         || type == Type::RangeDetect
                         ) && parser.extractChar(char0, QStringLiteral("char")))
                       || ((type == Type::Detect2Chars
                         || type == Type::RangeDetect
                         ) && parser.extractChar(char1, QStringLiteral("char1")))
                       || ((type == Type::AnyChar
                         || type == Type::RegExpr
                         || type == Type::StringDetect
                         || type == Type::WordDetect
                         || type == Type::keyword
                         ) && parser.extractString(string, QStringLiteral("String")))
                       || ((type == Type::IncludeRules)
                           && parser.extractXmlBool(includeAttrib, QStringLiteral("includeAttrib")))
                       || ((type == Type::Float
                         || type == Type::HlCHex
                         || type == Type::HlCOct
                         || type == Type::Int
                         || type == Type::keyword
                         || type == Type::WordDetect
                         ) && (parser.extractString(additionalDeliminator, QStringLiteral("additionalDeliminator"))
                            || parser.extractString(weakDeliminator, QStringLiteral("weakDeliminator"))))
                    ;
                    // clang-format on

                    success = parser.checkIfExtracted(isExtracted);

                    if (type == Type::LineContinue && char0 == QLatin1Char('\0')) {
                        char0 = QLatin1Char('\\');
                    }
                }

                return success;
            }

            bool checkMandoryAttributes(const QString &filename, QXmlStreamReader &xml)
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
        QString name;
        QString attribute;
        ContextName lineEndContext;
        ContextName lineEmptyContext;
        ContextName fallthroughContext;
        QVector<Rule> rules;
        XmlBool dynamic{};
        XmlBool fallthrough{};

        bool parseElement(const QString &filename, QXmlStreamReader &xml)
        {
            line = xml.lineNumber();

            bool success = true;

            for (auto &attr : xml.attributes()) {
                Parser parser{filename, xml, attr, success};
                XmlBool noIndentationBasedFolding{};

                const bool isExtracted = parser.extractString(name, QStringLiteral("name")) || parser.extractString(attribute, QStringLiteral("attribute"))
                    || parser.extractString(lineEndContext.name, QStringLiteral("lineEndContext"))
                    || parser.extractString(lineEmptyContext.name, QStringLiteral("lineEmptyContext"))
                    || parser.extractString(fallthroughContext.name, QStringLiteral("fallthroughContext"))
                    || parser.extractXmlBool(dynamic, QStringLiteral("dynamic")) || parser.extractXmlBool(fallthrough, QStringLiteral("fallthrough"))
                    || parser.extractXmlBool(noIndentationBasedFolding, QStringLiteral("noIndentationBasedFolding"));

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

            if (lineEndContext.name.isEmpty()) {
                qWarning() << filename << "line" << xml.lineNumber() << "missing attribute: lineEndContext";
                success = false;
            }

            return success;
        }
    };

    struct Version {
        int majorRevision;
        int minorRevision;

        Version(int majorRevision = 0, int minorRevision = 0)
            : majorRevision(majorRevision)
            , minorRevision(minorRevision)
        {
        }

        bool operator<(const Version &version) const
        {
            return majorRevision < version.majorRevision || (majorRevision == version.majorRevision && minorRevision < version.minorRevision);
        }
    };

    struct ItemDatas {
        struct Style {
            QString name;
            int line;

            friend uint qHash(const Style &style, uint seed = 0)
            {
                return qHash(style.name, seed);
            }

            friend bool operator==(const Style &style0, const Style &style1)
            {
                return style0.name == style1.name;
            }
        };

        QSet<Style> styleNames;

        bool parseElement(const QString &filename, QXmlStreamReader &xml)
        {
            bool success = true;

            QString name;
            QString defStyleNum;
            XmlBool boolean;

            for (auto &attr : xml.attributes()) {
                Parser parser{filename, xml, attr, success};

                const bool isExtracted = parser.extractString(name, QStringLiteral("name")) || parser.extractString(defStyleNum, QStringLiteral("defStyleNum"))
                    || parser.extractXmlBool(boolean, QStringLiteral("bold")) || parser.extractXmlBool(boolean, QStringLiteral("italic"))
                    || parser.extractXmlBool(boolean, QStringLiteral("underline")) || parser.extractXmlBool(boolean, QStringLiteral("strikeOut"))
                    || parser.extractXmlBool(boolean, QStringLiteral("spellChecking")) || parser.checkColor(QStringLiteral("color"))
                    || parser.checkColor(QStringLiteral("selColor")) || parser.checkColor(QStringLiteral("backgroundColor"))
                    || parser.checkColor(QStringLiteral("selBackgroundColor"));

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
        XmlBool casesensitive{};
        Version kateVersion{};
        QString kateVersionStr;
        QString languageName;
        QSet<const Definition *> referencedDefinitions;

        // Parse <keywords ...>
        bool parseKeywords(QXmlStreamReader &xml)
        {
            wordDelimiters.append(xml.attributes().value(QStringLiteral("additionalDeliminator")));
            wordDelimiters.remove(xml.attributes().value(QStringLiteral("weakDeliminator")));
            return true;
        }
    };

    // Parse <context>
    void processContextElement(QXmlStreamReader &xml)
    {
        Context context;
        m_success = context.parseElement(m_currentDefinition->filename, xml) && m_success;
        if (m_currentDefinition->firstContextName.isEmpty())
            m_currentDefinition->firstContextName = context.name;
        if (m_currentDefinition->contexts.contains(context.name)) {
            qWarning() << m_currentDefinition->filename << "line" << xml.lineNumber() << "duplicate context:" << context.name;
            m_success = false;
        }
        m_currentContext = &*m_currentDefinition->contexts.insert(context.name, context);
    }

    // Parse <list name="...">
    void processListElement(QXmlStreamReader &xml)
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
        QVector<const Context *> contexts;

        QMutableMapIterator<QString, Definition> def(m_definitions);
        while (def.hasNext()) {
            def.next();
            auto &definition = def.value();
            QMutableMapIterator<QString, Context> contextIt(definition.contexts);
            while (contextIt.hasNext()) {
                contextIt.next();
                for (auto &rule : contextIt.value().rules) {
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

                    usedContexts.clear();
                    usedContexts.insert(rule.context.context);
                    contexts.clear();
                    contexts.append(rule.context.context);

                    for (int i = 0; i < contexts.size(); ++i) {
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
        QVector<const Context *> contexts;

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

    //! Check contexts and rules
    bool checkContexts(const Definition &definition,
                       QSet<const Keywords *> &referencedKeywords,
                       QSet<ItemDatas::Style> &usedAttributeNames,
                       const QSet<const Context *> &usedContexts) const
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

            if (context.name.startsWith(QStringLiteral("#pop"))) {
                qWarning() << filename << "line" << context.line << "the context name must not start with '#pop':" << context.name;
                success = false;
            }

            if (!context.attribute.isEmpty())
                usedAttributeNames.insert({context.attribute, context.line});

            success = checkfallthrough(definition, context) && success;
            success = checkUreachableRules(definition.filename, context) && success;
            success = suggestRuleMerger(definition.filename, context) && success;

            for (const auto &rule : context.rules) {
                if (!rule.attribute.isEmpty())
                    usedAttributeNames.insert({rule.attribute, rule.line});
                success = checkLookAhead(rule) && success;
                success = checkStringDetect(rule) && success;
                success = checkAnyChar(rule) && success;
                success = checkKeyword(definition, rule, referencedKeywords) && success;
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
    //! - is not equivalent to DetectSpaces, DetectChar, Detect2Chars, StringDetect, DetectIdentifier, RangeDetect
    //! - has no unused captures
    //! - has no unnecessary quantifier with lookAhead
    bool checkRegExpr(const QString &filename, const Context::Rule &rule, const Context &context) const
    {
        if (rule.type == Context::Rule::Type::RegExpr) {
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

            auto reg = rule.string;
            reg.replace(QStringLiteral("{1}"), QString());

            // is DetectSpaces
            // optional ^ then \s, [\s], [\t ], [ \t] possibly in (...) or (?:...) followed by *, +
            static const QRegularExpression isDetectSpaces(
                QStringLiteral(R"(^\^?(?:\((?:\?:)?)?\^?(?:\\s|\[(?:\\s| (?:\t|\\t)|(?:\t|\\t) )\])\)?(?:[*+][*+?]?|[*+])?\)?\)?$)"));
            if (rule.string.contains(isDetectSpaces)) {
                char const *extraMsg = rule.string.contains(QLatin1Char('^')) ? "+ column=\"0\" or firstNonSpace=\"1\"" : "";
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by DetectSpaces / DetectChar / AnyChar" << extraMsg << ":"
                           << rule.string;
                return false;
            }

            // is RangeDetect
            static const QRegularExpression isRange(QStringLiteral(
                R"(^(\\(?:[^0BDPSWbdpswoux]|x[0-9a-fA-F]{2}|x\{[0-9a-fA-F]+\}|0\d\d|o\{[0-7]+\}|u[0-9a-fA-F]{4})|[^[.]|\[[^].\\]\])(?:\.|\[^\1\])\*[?*]?\1$)"));
            if (( rule.lookAhead == XmlBool::True
               || rule.minimal == XmlBool::True
               || rule.string.contains(QStringLiteral(".*?"))
               || rule.string.contains(QStringLiteral("[^"))
                ) && reg.contains(isRange)
            ) {
                qWarning() << filename << "line" << rule.line << "RegExpr should be replaced by RangeDetect:" << rule.string;
                return false;
            }

            // replace \c, \xhhh, \x{hhh...}, \0dd, \o{ddd}, \uhhhh, with _
            static const QRegularExpression sanitize1(
                QStringLiteral(R"(\\(?:[^0BDPSWbdpswoux]|x[0-9a-fA-F]{2}|x\{[0-9a-fA-F]+\}|0\d\d|o\{[0-7]+\}|u[0-9a-fA-F]{4}))"));
            reg.replace(sanitize1, QStringLiteral("_"));

            // use minimal or lazy operator
            static const QRegularExpression isMinimal(QStringLiteral(
                R"([.][*+][^][?+*()|$]*$)"));
            if (rule.lookAhead == XmlBool::True
             && rule.minimal != XmlBool::True
             && reg.contains(isMinimal)
            ) {
                qWarning() << filename << "line" << rule.line << "RegExpr should be have minimal=\"1\" or use lazy operator (i.g, '.*' -> '.*?'):" << rule.string;
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
                char const *extraMsg = rule.string.contains(QLatin1Char('^')) ? "+ column=\"0\" or firstNonSpace=\"1\"" : "";
                qWarning() << rule.filename << "line" << rule.line << "RegExpr should be replaced by StringDetect / Detect2Chars / DetectChar" << extraMsg
                           << ":" << rule.string;
                if (len != reg.size())
                    qWarning() << rule.filename << "line" << rule.line << "insensitive=\"1\" missing:" << rule.string;
                return false;
            }

            // column="0" or firstNonSpace="1"
            if (rule.column == -1 && rule.firstNonSpace != XmlBool::True) {
                // ^ without |
                // (^sas*) -> ok
                // (^sa|s*) -> ko
                // (^(sa|s*)) -> ok
                auto first = qAsConst(reg).begin();
                auto last = qAsConst(reg).end();
                int depth = 0;

                while (QLatin1Char('(') == *first) {
                    ++depth;
                    ++first;
                    if (QLatin1Char('?') == *first || QLatin1Char(':') == first[1]) {
                        first += 2;
                    }
                }

                if (QLatin1Char('^') == *first) {
                    const int bolDepth = depth;
                    bool replace = true;

                    while (++first != last) {
                        if (QLatin1Char('(') == *first) {
                            ++depth;
                        } else if (QLatin1Char(')') == *first) {
                            --depth;
                            if (depth < bolDepth) {
                                // (^a)? === (^a|) -> ko
                                if (first + 1 != last && QStringLiteral("*?").contains(first[1])) {
                                    replace = false;
                                    break;
                                }
                            }
                        } else if (QLatin1Char('|') == *first) {
                            // ignore '|' within subgroup
                            if (depth <= bolDepth) {
                                replace = false;
                                break;
                            }
                        }
                    }

                    if (replace) {
                        qWarning() << rule.filename << "line" << rule.line << "column=\"0\" or firstNonSpace=\"1\" missing with RegExpr:" << rule.string;
                        return false;
                    }
                }
            }

            // add ^ with column=0
            if (rule.column == 0 && !rule.isDotRegex) {
                bool hasStartOfLine = false;
                auto first = qAsConst(reg).begin();
                auto last = qAsConst(reg).end();
                for (; first != last; ++first) {
                    if (*first == QLatin1Char('^')) {
                        hasStartOfLine = true;
                        break;
                    }
                    else if (*first == QLatin1Char('(')) {
                        if (last - first >= 3 && first[1] == QLatin1Char('?') && first[2] == QLatin1Char(':')) {
                            first += 2;
                        }
                    }
                    else {
                        break;
                    }
                }

                if (!hasStartOfLine) {
                    qWarning() << rule.filename << "line" << rule.line << "start of line missing in the pattern with column=\"0\" (i.e. abc -> ^abc):" << rule.string;
                    return false;
                }
            }

            bool useCapture = false;

            // detection of unnecessary capture
            if (regexp.captureCount()) {
                auto maximalCapture = [](const QString(&referenceNames)[9], const QString &s) {
                    int maxCapture = 9;
                    while (maxCapture && !s.contains(referenceNames[maxCapture - 1])) {
                        --maxCapture;
                    }
                    return maxCapture;
                };

                int maxCaptureUsed = 0;
                // maximal dynamic reference
                if (rule.context.context && !rule.context.stay) {
                    for (const auto &nextRule : rule.context.context->rules) {
                        if (nextRule.dynamic == XmlBool::True) {
                            static const QString cap[]{
                                QStringLiteral("%1"),
                                QStringLiteral("%2"),
                                QStringLiteral("%3"),
                                QStringLiteral("%4"),
                                QStringLiteral("%5"),
                                QStringLiteral("%6"),
                                QStringLiteral("%7"),
                                QStringLiteral("%8"),
                                QStringLiteral("%9"),
                            };
                            int maxDynamicCapture = maximalCapture(cap, nextRule.string);
                            maxCaptureUsed = std::max(maxCaptureUsed, maxDynamicCapture);
                        }
                    }
                }

                static const QString num1[]{
                    QStringLiteral("\\1"),
                    QStringLiteral("\\2"),
                    QStringLiteral("\\3"),
                    QStringLiteral("\\4"),
                    QStringLiteral("\\5"),
                    QStringLiteral("\\6"),
                    QStringLiteral("\\7"),
                    QStringLiteral("\\8"),
                    QStringLiteral("\\9"),
                };
                static const QString num2[]{
                    QStringLiteral("\\g1"),
                    QStringLiteral("\\g2"),
                    QStringLiteral("\\g3"),
                    QStringLiteral("\\g4"),
                    QStringLiteral("\\g5"),
                    QStringLiteral("\\g6"),
                    QStringLiteral("\\g7"),
                    QStringLiteral("\\g8"),
                    QStringLiteral("\\g9"),
                };
                const int maxBackReference = std::max(maximalCapture(num1, rule.string), maximalCapture(num1, rule.string));

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
                static const QRegularExpression isInsensitiveDetectIdentifier(
                    QStringLiteral(R"(^(\((\?:)?)?\[((a-z|_){2}|(A-Z|_){2})\]([+][*?]?)?\[((0-9|a-z|_){3}|(0-9|A-Z|_){3})\][*][*?]?(\))?$)"));
                static const QRegularExpression isSensitiveDetectIdentifier(
                    QStringLiteral(R"(^(\((\?:)?)?\[(a-z|A-Z|_){3}\]([+][*?]?)?\[(0-9|a-z|A-Z|_){4}\][*][*?]?(\))?$)"));
                auto &isDetectIdentifier = (rule.insensitive == XmlBool::True) ? isInsensitiveDetectIdentifier : isSensitiveDetectIdentifier;
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

                auto ruleFilename = (filename == rule.filename) ? QString() : QStringLiteral("in ") + rule.filename;
                if (i == context.rules.size()) {
                    if (rule.lookAhead == XmlBool::True && rule.firstNonSpace != XmlBool::True && rule.column == -1 && rule.beginRegion.isEmpty()
                        && rule.endRegion.isEmpty() && !useCapture) {
                        qWarning() << filename << "context line" << context.line << ": RegExpr line" << rule.line << ruleFilename
                                   << "should be replaced by fallthroughContext:" << rule.string;
                    }
                } else {
                    auto &nextRule = context.rules[i];
                    auto nextRuleFilename = (filename == nextRule.filename) ? QString() : QStringLiteral("in ") + nextRule.filename;
                    qWarning() << filename << "context line" << context.line << "contains unreachable element line" << nextRule.line << nextRuleFilename
                               << "because a dot RegExpr is used line" << rule.line << ruleFilename;
                }

                // unnecessary quantifier
                static const QRegularExpression unnecessaryQuantifier1(QStringLiteral(
                    R"([*+?]([.][*+?]{0,2})?$)"));
                static const QRegularExpression unnecessaryQuantifier2(QStringLiteral(
                    R"([*+?]([.][*+?]{0,2})?[)]*$)"));
                auto &unnecessaryQuantifier = useCapture ? unnecessaryQuantifier1 : unnecessaryQuantifier2;
                if (rule.lookAhead == XmlBool::True && rule.minimal != XmlBool::True && reg.contains(unnecessaryQuantifier)) {
                    qWarning() << filename << "line" << rule.line << "Last quantifier is not necessary (i.g., 'xyz*' -> 'xy', 'xyz+.' -> 'xyz.'):"
                               << rule.string;
                    return false;
                }
            }
        }

        return true;
    }

    // Parse and check <emptyLine>
    bool parseEmptyLine(const QString &filename, QXmlStreamReader &xml)
    {
        bool success = true;

        QString pattern;
        XmlBool casesensitive{};

        for (auto &attr : xml.attributes()) {
            Parser parser{filename, xml, attr, success};

            const bool isExtracted =
                parser.extractString(pattern, QStringLiteral("regexpr")) || parser.extractXmlBool(casesensitive, QStringLiteral("casesensitive"));

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
        const int azOffset = std::max(pattern.indexOf(QStringLiteral("A-z")), pattern.indexOf(QStringLiteral("a-Z")));
        if (azOffset >= 0) {
            qWarning() << filename << "line" << line << "broken regex:" << pattern << "problem: [a-Z] or [A-z] at offset" << azOffset;
            return false;
        }

        return true;
    }

    //! Search for rules with lookAhead="true" and context="#stay".
    //! This would cause an infinite loop.
    bool checkfallthrough(const Definition &definition, const Context &context) const
    {
        bool success = true;

        if (!context.fallthroughContext.name.isEmpty()) {
            if (context.fallthroughContext.stay) {
                qWarning() << definition.filename << "line" << context.line << "possible infinite loop due to fallthroughContext=\"#stay\" in context "
                           << context.name;
                success = false;
            }

            const bool mandatoryFallthroughAttribute = definition.kateVersion < Version{5, 62};
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

        return success;
    }

    //! Search for additionalDeliminator/weakDeliminator which has no effect.
    bool checkDelimiters(const Definition &definition, const Context::Rule &rule) const
    {
        if (rule.additionalDeliminator.isEmpty() && rule.weakDeliminator.isEmpty())
            return true;

        bool success = true;

        if (definition.kateVersion < Version{5, 79}) {
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

    //! Search for rules with lookAhead="true" and context="#stay".
    //! This would cause an infinite loop.
    bool checkKeyword(const Definition &definition, const Context::Rule &rule, QSet<const Keywords *> &referencedKeywords) const
    {
        if (rule.type == Context::Rule::Type::keyword) {
            auto it = definition.keywordsList.find(rule.string);
            if (it != definition.keywordsList.end()) {
                referencedKeywords.insert(&*it);
            } else {
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

    //! Check that StringDetect contains more that 2 characters
    //! Fix with following command:
    //! \code
    //!   sed -E
    //!   '/StringDetect/{/dynamic="(1|true)|insensitive="(1|true)/!{s/StringDetect(.*)String="(.|&lt;|&gt;|&quot;|&amp;)(.|&lt;|&gt;|&quot;|&amp;)"/Detect2Chars\1char="\2"
    //!   char1="\3"/;t;s/StringDetect(.*)String="(.|&lt;|&gt;|&quot;|&amp;)"/DetectChar\1char="\2"/}}' -i file.xml...
    //! \endcode
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
            } else {
                if (rule.string.size() <= 1) {
                    const auto replacement = rule.insensitive == XmlBool::True ? QStringLiteral("AnyChar") : QStringLiteral("DetectChar");
                    qWarning() << rule.filename << "line" << rule.line << "StringDetect should be replaced by" << replacement;
                    return false;
                }

                if (rule.string.size() <= 2 && rule.insensitive != XmlBool::True) {
                    qWarning() << rule.filename << "line" << rule.line << "StringDetect should be replaced by Detect2Chars";
                    return false;
                }
            }
        }
        return true;
    }

    //! Check that AnyChar contains more that 1 character
    bool checkAnyChar(const Context::Rule &rule) const
    {
        if (rule.type == Context::Rule::Type::AnyChar && rule.string.size() <= 1) {
            qWarning() << rule.filename << "line" << rule.line << "AnyChar should be replaced by DetectChar";
            return false;
        }
        return true;
    }

    //! Check \<include> and delimiter in a keyword list
    bool checkKeywordsList(const Definition &definition, QSet<const Keywords *> &referencedKeywords) const
    {
        bool success = true;

        bool includeNotSupport = (definition.kateVersion < Version{5, 53});
        QMapIterator<QString, Keywords> keywordsIt(definition.keywordsList);
        while (keywordsIt.hasNext()) {
            keywordsIt.next();

            for (const auto &include : keywordsIt.value().items.includes) {
                if (includeNotSupport) {
                    qWarning() << definition.filename << "line" << include.line
                               << "<include> is only available since version \"5.53\". Please, increase kateversion.";
                    success = false;
                }
                success = checkKeywordInclude(definition, include, referencedKeywords) && success;
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
    bool checkKeywordInclude(const Definition &definition, const Keywords::Items::Item &include, QSet<const Keywords *> &referencedKeywords) const
    {
        bool containsKeywordName = true;
        int const idx = include.content.indexOf(QStringLiteral("##"));
        if (idx == -1) {
            auto it = definition.keywordsList.find(include.content);
            containsKeywordName = (it != definition.keywordsList.end());
            if (containsKeywordName) {
                referencedKeywords.insert(&*it);
            }
        } else {
            auto defName = include.content.mid(idx + 2);
            auto listName = include.content.left(idx);
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
    bool checkUreachableRules(const QString &filename, const Context &context) const
    {
        struct RuleAndInclude {
            const Context::Rule *rule;
            const Context::Rule *includeRules;

            explicit operator bool() const
            {
                return rule;
            }
        };

        // Associate QChar with RuleAndInclude
        struct CharTable {
            /// Search RuleAndInclude associated with @p c.
            RuleAndInclude find(QChar c) const
            {
                if (c.unicode() < 128)
                    return m_asciiMap[c.unicode()];
                auto it = m_utf8Map.find(c);
                return it == m_utf8Map.end() ? RuleAndInclude{nullptr, nullptr} : it.value();
            }

            /// Search RuleAndInclude associated with the characters of @p s.
            /// \return an empty QVector when at least one character is not found.
            QVector<RuleAndInclude> find(QStringView s) const
            {
                QVector<RuleAndInclude> result;

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
                if (c.unicode() < 128)
                    m_asciiMap[c.unicode()] = {&rule, includeRule};
                else
                    m_utf8Map[c] = {&rule, includeRule};
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
                if (rule.firstNonSpace == XmlBool::True)
                    appendTable(tables.charsFirstNonSpace);

                if (rule.column == 0)
                    appendTable(tables.charsColumn0);
                else if (rule.column > 0)
                    appendTable(tables.charsColumnGreaterThan0[rule.column]);

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
                    if (auto ruleAndInclude = m_charTables[i]->find(c))
                        return ruleAndInclude;
                }
                return RuleAndInclude{nullptr, nullptr};
            }

            /// Search RuleAndInclude associated with the characters of @p s.
            /// \return an empty QVector when at least one character is not found.
            QVector<RuleAndInclude> find(QStringView s) const
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
                return QVector<RuleAndInclude>();
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

        // Iterates over all the rules, including those in includedRules
        struct RuleIterator {
            RuleIterator(const QVector<Context::Rule> &rules, const Context::Rule &endRule)
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
                    if (m_i2 != m_rules[m_i].includedRules.size()) {
                        return (*m_includedRules)[m_i2];
                    }
                    ++m_i;
                    m_includedRules = nullptr;
                }

                // if is a includedRules
                while (m_i < m_end && m_rules[m_i].type == Context::Rule::Type::IncludeRules) {
                    if (m_rules[m_i].includedRules.size()) {
                        m_i2 = 0;
                        m_includedRules = &m_rules[m_i].includedRules;
                        return (*m_includedRules)[m_i2];
                    }
                    ++m_i;
                }

                if (m_i < m_end) {
                    ++m_i;
                    return &m_rules[m_i - 1];
                }

                return nullptr;
            }

            /// \return current IncludeRules or nullptr
            const Context::Rule *currentIncludeRules() const
            {
                return m_includedRules ? &m_rules[m_i] : nullptr;
            }

        private:
            int m_i = 0;
            int m_i2;
            int m_end;
            const QVector<Context::Rule> &m_rules;
            const QVector<const Context::Rule *> *m_includedRules = nullptr;
        };

        // Dot regex container that satisfies firstNonSpace and column.
        struct DotRegex {
            /// Append a dot regex rule.
            void append(const Context::Rule &rule, const Context::Rule *includedRule)
            {
                auto array = extractDotRegexes(rule);
                if (array[0])
                    *array[0] = {&rule, includedRule};
                if (array[1])
                    *array[1] = {&rule, includedRule};
            }

            /// Search dot regex which hides @p rule
            RuleAndInclude find(const Context::Rule &rule)
            {
                auto array = extractDotRegexes(rule);
                if (array[0])
                    return *array[0];
                if (array[1])
                    return *array[1];
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
                    if (rule.firstNonSpace == XmlBool::True)
                        ret[0] = &dotRegexFirstNonSpace;

                    if (rule.column == 0)
                        ret[1] = &dotRegexColumn0;
                    else if (rule.column > 0)
                        ret[1] = &dotRegexColumnGreaterThan0[rule.column];
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

        RuleAndInclude intRule{};
        RuleAndInclude floatRule{};
        RuleAndInclude hlCCharRule{};
        RuleAndInclude hlCOctRule{};
        RuleAndInclude hlCHexRule{};
        RuleAndInclude hlCStringCharRule{};

        // Contains includedRules and included includedRules
        QMap<Context const*, RuleAndInclude> includeContexts;

        DotRegex dotRegex;

        for (const Context::Rule &rule : context.rules) {
            bool isUnreachable = false;
            QVector<RuleAndInclude> unreachableBy;

            // declare rule as unreacheable if ruleAndInclude is not empty
            auto updateUnreachable1 = [&](RuleAndInclude ruleAndInclude) {
                if (ruleAndInclude) {
                    isUnreachable = true;
                    unreachableBy.append(ruleAndInclude);
                }
            };

            // declare rule as unreacheable if ruleAndIncludes is not empty
            auto updateUnreachable2 = [&](const QVector<RuleAndInclude> &ruleAndIncludes) {
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
                updateUnreachable2(tables.find(QStringLiteral(" \t")));
                tables.removeNonSpecialWhenSpecial();
                tables.append(QLatin1Char(' '), rule);
                tables.append(QLatin1Char('\t'), rule);
                break;
            }

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::HlCChar:
                updateUnreachable1(CharTableArray(detectChars, rule).find(QLatin1Char('\'')));
                updateUnreachable1(hlCCharRule);
                hlCCharRule = {&rule, nullptr};
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::HlCHex:
                updateUnreachable1(CharTableArray(detectChars, rule).find(QLatin1Char('0')));
                updateUnreachable1(hlCHexRule);
                hlCHexRule = {&rule, nullptr};
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::HlCOct:
                updateUnreachable1(CharTableArray(detectChars, rule).find(QLatin1Char('0')));
                updateUnreachable1(hlCOctRule);
                hlCOctRule = {&rule, nullptr};
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::HlCStringChar:
                updateUnreachable1(CharTableArray(detectChars, rule).find(QLatin1Char('\\')));
                updateUnreachable1(hlCStringCharRule);
                hlCStringCharRule = {&rule, nullptr};
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::Int:
                updateUnreachable2(CharTableArray(detectChars, rule).find(QStringLiteral("0123456789")));
                updateUnreachable1(intRule);
                intRule = {&rule, nullptr};
                break;

            // check if hidden by DetectChar/AnyChar
            case Context::Rule::Type::Float:
                updateUnreachable2(CharTableArray(detectChars, rule).find(QStringLiteral("0123456789.")));
                updateUnreachable1(floatRule);
                floatRule = {&rule, nullptr};
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
                    RuleIterator ruleIterator(context.rules, rule);
                    while (const auto *rulePtr = ruleIterator.next()) {
                        if (isUnreachable)
                            break;
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
                RuleIterator ruleIterator(context.rules, rule);
                while (const auto *rulePtr = ruleIterator.next()) {
                    if (isUnreachable)
                        break;
                    const auto &rule2 = *rulePtr;
                    if (rule2.type == Context::Rule::Type::RegExpr && isCompatible(rule2) && rule.insensitive == rule2.insensitive
                        && rule.dynamic == rule2.dynamic && rule.string.startsWith(rule2.string)) {
                        updateUnreachable1({&rule2, ruleIterator.currentIncludeRules()});
                    }
                }

                Q_FALLTHROUGH();
            }
            // check if a rule does not have another rule as a prefix
            case Context::Rule::Type::WordDetect:
            case Context::Rule::Type::StringDetect: {
                // check that dynamic `rule` does not have another dynamic StringDetect as a prefix
                if (rule.type == Context::Rule::Type::StringDetect && rule.dynamic == XmlBool::True) {
                    RuleIterator ruleIterator(context.rules, rule);
                    while (const auto *rulePtr = ruleIterator.next()) {
                        if (isUnreachable)
                            break;

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
                    s = s.left(result.capturedLength());
                }

                QString sanitizedRegex;
                // truncate to special character with RegExpr.
                // If regexp contains '|', `s` becomes empty.
                if (rule.type == Context::Rule::Type::RegExpr) {
                    static const QRegularExpression regularChars(QStringLiteral(R"(^(?:[^.?*+^$[{(\\|]+|\\[-.?*+^$[\]{}()\\|]+|\[[^^\\]\])+)"));
                    static const QRegularExpression sanitizeChars(QStringLiteral(R"(\\([-.?*+^$[\]{}()\\|])|\[([^^\\])\])"));
                    const qsizetype result = regularChars.match(rule.string).capturedLength();
                    const qsizetype pos = qMin(result, s.size());
                    if (rule.string.indexOf(QLatin1Char('|'), pos) < pos) {
                        sanitizedRegex = rule.string.left(qMin(result, s.size()));
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
                }

                // check if Detect2Chars, StringDetect, WordDetect is not a prefix of s
                if (s.size() > 0 && !isUnreachable) {
                    // combination of uppercase and lowercase
                    RuleAndInclude detect2CharsInsensitives[]{{}, {}, {}, {}};

                    RuleIterator ruleIterator(context.rules, rule);
                    while (const auto *rulePtr = ruleIterator.next()) {
                        if (isUnreachable)
                            break;
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
                RuleIterator ruleIterator(context.rules, rule);
                while (const auto *rulePtr = ruleIterator.next()) {
                    if (isUnreachable)
                        break;
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
                if (auto &ruleAndInclude = includeContexts[rule.context.context]) {
                    updateUnreachable1(ruleAndInclude);
                }
                else {
                    ruleAndInclude.rule = &rule;
                }

                for (const auto *rulePtr : rule.includedIncludeRules) {
                    includeContexts.insert(rulePtr->context.context, RuleAndInclude{rulePtr, &rule});
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
                        auto &chars4 = (rule.dynamic != XmlBool::True) ? detectChars : dynamicDetectChars;
                        auto tables = CharTableArray(chars4, rule2);
                        tables.removeNonSpecialWhenSpecial();
                        tables.append(rule2.char0, rule2, &rule);
                        break;
                    }

                    case Context::Rule::Type::DetectSpaces: {
                        auto tables = CharTableArray(detectChars, rule2);
                        tables.removeNonSpecialWhenSpecial();
                        tables.append(QLatin1Char(' '), rule2, &rule);
                        tables.append(QLatin1Char('\t'), rule2, &rule);
                        break;
                    }

                    case Context::Rule::Type::HlCChar:
                        hlCCharRule = {&rule2, &rule};
                        break;

                    case Context::Rule::Type::HlCHex:
                        hlCHexRule = {&rule2, &rule};
                        break;

                    case Context::Rule::Type::HlCOct:
                        hlCOctRule = {&rule2, &rule};
                        break;

                    case Context::Rule::Type::HlCStringChar:
                        hlCStringCharRule = {&rule2, &rule};
                        break;

                    case Context::Rule::Type::Int:
                        intRule = {&rule2, &rule};
                        break;

                    case Context::Rule::Type::Float:
                        floatRule = {&rule2, &rule};
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

                    case Context::Rule::Type::WordDetect:
                    case Context::Rule::Type::StringDetect:
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

            case Context::Rule::Type::DetectIdentifier:
            case Context::Rule::Type::Unknown:
                break;
            }

            if (isUnreachable) {
                success = false;
                QString message;
                message.reserve(128);
                for (auto &ruleAndInclude : unreachableBy) {
                    message += QStringLiteral("line ");
                    if (ruleAndInclude.includeRules) {
                        message += QString::number(ruleAndInclude.includeRules->line);
                        message += QStringLiteral(" [by '");
                        message += ruleAndInclude.includeRules->context.name;
                        message += QStringLiteral("' line ");
                        message += QString::number(ruleAndInclude.rule->line);
                        if (ruleAndInclude.includeRules->filename != ruleAndInclude.rule->filename) {
                            message += QStringLiteral(" (");
                            message += ruleAndInclude.rule->filename;
                            message += QLatin1Char(')');
                        }
                        message += QLatin1Char(']');
                    } else {
                        message += QString::number(ruleAndInclude.rule->line);
                    }
                    message += QStringLiteral(", ");
                }
                message.chop(2);
                qWarning() << filename << "line" << rule.line << "unreachable element by" << message;
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

        if (context.rules.isEmpty())
            return success;

        auto it = context.rules.begin();
        const auto end = context.rules.end() - 1;

        for (; it < end; ++it) {
            auto& rule1 = *it;
            auto& rule2 = it[1];

            auto isCommonCompatible = [&]{
                return rule1.attribute == rule2.attribute
                    && rule1.beginRegion == rule2.beginRegion
                    && rule1.endRegion == rule2.endRegion
                    && rule1.lookAhead == rule2.lookAhead
                    && rule1.firstNonSpace == rule2.firstNonSpace
                    && rule1.context.context == rule2.context.context
                    && rule1.context.popCount == rule2.context.popCount
                    ;
            };

            switch (rule1.type) {
                // request to merge AnyChar/DetectChar
                case Context::Rule::Type::AnyChar:
                case Context::Rule::Type::DetectChar:
                    if ((rule2.type == Context::Rule::Type::AnyChar || rule2.type == Context::Rule::Type::DetectChar) && isCommonCompatible() && rule1.column == rule2.column) {
                        qWarning() << filename << "line" << rule2.line << "can be merged as AnyChar with the previous rule";
                        success = false;
                    }
                    break;

                // request to merge multiple RegExpr
                case Context::Rule::Type::RegExpr:
                    if (rule2.type == Context::Rule::Type::RegExpr && isCommonCompatible() && rule1.dynamic == rule2.dynamic && (rule1.column == rule2.column || (rule1.column <= 0 && rule2.column <= 0))) {
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
                case Context::Rule::Type::StringDetect:
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
    void resolveContextName(Definition &definition, const Context &context, ContextName &contextName, int line)
    {
        QString name = contextName.name;
        if (name.isEmpty()) {
            contextName.stay = true;
        } else if (name.startsWith(QStringLiteral("#stay"))) {
            name = name.mid(5);
            contextName.stay = true;
            contextName.context = &context;
            if (!name.isEmpty()) {
                qWarning() << definition.filename << "line" << line << "invalid context in" << context.name;
                m_success = false;
            }
        } else {
            while (name.startsWith(QStringLiteral("#pop"))) {
                name = name.mid(4);
                ++contextName.popCount;
            }

            if (contextName.popCount && !name.isEmpty()) {
                if (name.startsWith(QLatin1Char('!')) && name.size() > 1) {
                    name = name.mid(1);
                } else {
                    qWarning() << definition.filename << "line" << line << "'!' missing between '#pop' and context name" << context.name;
                    m_success = false;
                }
            }

            if (!name.isEmpty()) {
                const int idx = name.indexOf(QStringLiteral("##"));
                if (idx == -1) {
                    auto it = definition.contexts.find(name);
                    if (it != definition.contexts.end())
                        contextName.context = &*it;
                } else {
                    auto defName = name.mid(idx + 2);
                    auto listName = name.left(idx);
                    auto it = m_definitions.find(defName);
                    if (it != m_definitions.end()) {
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
    Definition *m_currentDefinition = nullptr;
    Keywords *m_currentKeywords = nullptr;
    Context *m_currentContext = nullptr;
    bool m_success = true;
};

namespace
{
QStringList readListing(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QStringList();
    }

    QXmlStreamReader xml(&file);
    QStringList listing;
    while (!xml.atEnd()) {
        xml.readNext();

        // add only .xml files, no .json or stuff
        if (xml.isCharacters() && xml.text().toString().contains(QLatin1String(".xml"))) {
            listing.append(xml.text().toString());
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML error while reading" << fileName << " - " << qPrintable(xml.errorString()) << "@ offset" << xml.characterOffset();
        listing.clear();
    }

    return listing;
}

/**
 * check if the "extensions" attribute have valid wildcards
 * @param extensions extensions string to check
 * @return valid?
 */
bool checkExtensions(const QString &extensions)
{
    // get list of extensions
    const QStringList extensionParts = extensions.split(QLatin1Char(';'), Qt::SkipEmptyParts);

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
            if (c == QLatin1Char('.') || c == QLatin1Char('-') || c == QLatin1Char('_') || c == QLatin1Char('+')) {
                continue;
            }

            // only allowed wildcard things: '?' and '*'
            if (c == QLatin1Char('?') || c == QLatin1Char('*')) {
                continue;
            }

            qWarning() << "invalid character" << c << "seen in extensions wildcard";
            return false;
        }
    }

    // all checks passed
    return true;
}

}

int main(int argc, char *argv[])
{
    // get app instance
    QCoreApplication app(argc, argv);

    // ensure enough arguments are passed
    if (app.arguments().size() < 3)
        return 1;

#ifdef QT_XMLPATTERNS_LIB
    // open schema
    QXmlSchema schema;
    if (!schema.load(QUrl::fromLocalFile(app.arguments().at(2))))
        return 2;
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
    const QStringList textAttributes = QStringList() << QStringLiteral("name") << QStringLiteral("section") << QStringLiteral("mimetype")
                                                     << QStringLiteral("extensions") << QStringLiteral("style") << QStringLiteral("author")
                                                     << QStringLiteral("license") << QStringLiteral("indenter");

    // index all given highlightings
    HlFilesChecker filesChecker;
    QVariantMap hls;
    int anyError = 0;
    for (const QString &hlFilename : qAsConst(hlFilenames)) {
        QFile hlFile(hlFilename);
        if (!hlFile.open(QIODevice::ReadOnly)) {
            qWarning("Failed to open %s", qPrintable(hlFilename));
            anyError = 3;
            continue;
        }

#ifdef QT_XMLPATTERNS_LIB
        // validate against schema
        QXmlSchemaValidator validator(schema);
        if (!validator.validate(&hlFile, QUrl::fromLocalFile(hlFile.fileName()))) {
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
        for (const QString &attribute : qAsConst(textAttributes)) {
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

        // remember hl
        hls[QFileInfo(hlFile).fileName()] = hl;

        const QString hlName = hl[QStringLiteral("name")].toString();

        filesChecker.setDefinition(xml.attributes().value(QStringLiteral("kateversion")), hlFilename, hlName);

        // scan for broken regex or keywords with spaces
        while (!xml.atEnd()) {
            xml.readNext();
            filesChecker.processElement(xml);
        }
    }

    filesChecker.resolveContexts();

    if (!filesChecker.check())
        anyError = 7;

    // bail out if any problem was seen
    if (anyError)
        return anyError;

    // create outfile, after all has worked!
    QFile outFile(app.arguments().at(1));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return 9;

    // write out json
    outFile.write(QCborValue::fromVariant(QVariant(hls)).toCbor());

    // be done
    return 0;
}
