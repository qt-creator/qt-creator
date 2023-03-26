/*
    SPDX-FileCopyrightText: 2021 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "highlightingdata_p.hpp"
#include "ksyntaxhighlighting_logging.h"
#include "xml_p.h"

#include <QXmlStreamReader>
#include <QStringView>

using namespace KSyntaxHighlighting;

template<class Data, class... Args>
static void initRuleData(Data &data, Args &&...args)
{
    new (&data) Data{std::move(args)...};
}

static Qt::CaseSensitivity attrToCaseSensitivity(QStringView str)
{
    return Xml::attrToBool(str) ? Qt::CaseInsensitive : Qt::CaseSensitive;
}

static HighlightingContextData::Rule::WordDelimiters loadAdditionalWordDelimiters(QXmlStreamReader &reader)
{
    return HighlightingContextData::Rule::WordDelimiters{
        reader.attributes().value(QLatin1String("additionalDeliminator")).toString(),
        reader.attributes().value(QLatin1String("weakDeliminator")).toString(),
    };
}

static bool checkIsNotEmpty(QStringView str, const char *attrName, const QString &defName, QXmlStreamReader &reader)
{
    if (!str.isEmpty()) {
        return true;
    }

    qCWarning(Log) << defName << "at line" << reader.lineNumber() << ": " << attrName << "attribute is empty";
    return false;
}

static bool checkIsChar(QStringView str, const char *attrName, const QString &defName, QXmlStreamReader &reader)
{
    if (str.size() == 1) {
        return true;
    }

    qCWarning(Log) << defName << "at line" << reader.lineNumber() << ": " << attrName << "attribute must contain exactly 1 character";
    return false;
}

static bool loadRule(const QString &defName, HighlightingContextData::Rule &rule, QXmlStreamReader &reader)
{
    using Rule = HighlightingContextData::Rule;

    QStringView name = reader.name();
    const auto attrs = reader.attributes();
    bool isIncludeRules = false;

    if (name == QLatin1String("DetectChar")) {
        const auto s = attrs.value(QLatin1String("char"));
        if (!checkIsChar(s, "char", defName, reader)) {
            return false;
        }
        const QChar c = s.at(0);
        const bool dynamic = Xml::attrToBool(attrs.value(QLatin1String("dynamic")));

        initRuleData(rule.data.detectChar, c, dynamic);
        rule.type = Rule::Type::DetectChar;
    } else if (name == QLatin1String("RegExpr")) {
        const auto pattern = attrs.value(QLatin1String("String"));
        if (!checkIsNotEmpty(pattern, "String", defName, reader)) {
            return false;
        }

        const auto isCaseInsensitive = attrToCaseSensitivity(attrs.value(QLatin1String("insensitive")));
        const auto isMinimal = Xml::attrToBool(attrs.value(QLatin1String("minimal")));
        const auto dynamic = Xml::attrToBool(attrs.value(QLatin1String("dynamic")));

        initRuleData(rule.data.regExpr, pattern.toString(), isCaseInsensitive, isMinimal, dynamic);
        rule.type = Rule::Type::RegExpr;
    } else if (name == QLatin1String("IncludeRules")) {
        const auto context = attrs.value(QLatin1String("context"));
        if (!checkIsNotEmpty(context, "context", defName, reader)) {
            return false;
        }
        const bool includeAttribute = Xml::attrToBool(attrs.value(QLatin1String("includeAttrib")));

        initRuleData(rule.data.includeRules, context.toString(), includeAttribute);
        rule.type = Rule::Type::IncludeRules;
        isIncludeRules = true;
    } else if (name == QLatin1String("Detect2Chars")) {
        const auto s1 = attrs.value(QLatin1String("char"));
        const auto s2 = attrs.value(QLatin1String("char1"));
        if (!checkIsChar(s1, "char", defName, reader)) {
            return false;
        }
        if (!checkIsChar(s2, "char1", defName, reader)) {
            return false;
        }

        initRuleData(rule.data.detect2Chars, s1.at(0), s2.at(0));
        rule.type = Rule::Type::Detect2Chars;
    } else if (name == QLatin1String("keyword")) {
        const auto s = attrs.value(QLatin1String("String"));
        if (!checkIsNotEmpty(s, "String", defName, reader)) {
            return false;
        }
        Qt::CaseSensitivity caseSensitivityOverride = Qt::CaseInsensitive;
        bool hasCaseSensitivityOverride = false;

        /**
         * we might overwrite the case sensitivity
         * then we need to init the list for lookup of that sensitivity setting
         */
        if (attrs.hasAttribute(QLatin1String("insensitive"))) {
            hasCaseSensitivityOverride = true;
            caseSensitivityOverride = attrToCaseSensitivity(attrs.value(QLatin1String("insensitive")));
        }

        initRuleData(rule.data.keyword, s.toString(), loadAdditionalWordDelimiters(reader), caseSensitivityOverride, hasCaseSensitivityOverride);
        rule.type = Rule::Type::Keyword;
    } else if (name == QLatin1String("DetectSpaces")) {
        rule.type = Rule::Type::DetectSpaces;
    } else if (name == QLatin1String("StringDetect")) {
        const auto string = attrs.value(QLatin1String("String"));
        if (!checkIsNotEmpty(string, "String", defName, reader)) {
            return false;
        }
        const auto caseSensitivity = attrToCaseSensitivity(attrs.value(QLatin1String("insensitive")));
        const auto dynamic = Xml::attrToBool(attrs.value(QLatin1String("dynamic")));
        const bool isSensitive = (caseSensitivity == Qt::CaseSensitive);

        // String can be replaced with DetectChar or AnyChar
        if (!dynamic && string.size() == 1) {
            QChar c = string.at(0);
            if (isSensitive || c.toLower() == c.toUpper()) {
                initRuleData(rule.data.detectChar, c, dynamic);
                rule.type = Rule::Type::DetectChar;
            } else {
                initRuleData(rule.data.anyChar, c.toLower() + c.toUpper());
                rule.type = Rule::Type::AnyChar;
            }
        }
        // String can be replaced with Detect2Chars
        else if (isSensitive && !dynamic && string.size() == 2) {
            initRuleData(rule.data.detect2Chars, string.at(0), string.at(1));
            rule.type = Rule::Type::Detect2Chars;
        } else {
            initRuleData(rule.data.stringDetect, string.toString(), caseSensitivity, dynamic);
            rule.type = Rule::Type::StringDetect;
        }
    } else if (name == QLatin1String("WordDetect")) {
        const auto word = attrs.value(QLatin1String("String"));
        if (!checkIsNotEmpty(word, "String", defName, reader)) {
            return false;
        }
        const auto caseSensitivity = attrToCaseSensitivity(attrs.value(QLatin1String("insensitive")));

        initRuleData(rule.data.wordDetect, word.toString(), loadAdditionalWordDelimiters(reader), caseSensitivity);
        rule.type = Rule::Type::WordDetect;
    } else if (name == QLatin1String("AnyChar")) {
        const auto chars = attrs.value(QLatin1String("String"));
        if (!checkIsNotEmpty(chars, "String", defName, reader)) {
            return false;
        }

        // AnyChar can be replaced with DetectChar
        if (chars.size() == 1) {
            initRuleData(rule.data.detectChar, chars.at(0), false);
            rule.type = Rule::Type::DetectChar;
        } else {
            initRuleData(rule.data.anyChar, chars.toString());
            rule.type = Rule::Type::AnyChar;
        }
    } else if (name == QLatin1String("DetectIdentifier")) {
        rule.type = Rule::Type::DetectIdentifier;
    } else if (name == QLatin1String("LineContinue")) {
        const auto s = attrs.value(QLatin1String("char"));
        const QChar c = s.isEmpty() ? QLatin1Char('\\') : s.at(0);

        initRuleData(rule.data.lineContinue, c);
        rule.type = Rule::Type::LineContinue;
    } else if (name == QLatin1String("Int")) {
        initRuleData(rule.data.detectInt, loadAdditionalWordDelimiters(reader));
        rule.type = Rule::Type::Int;
    } else if (name == QLatin1String("Float")) {
        initRuleData(rule.data.detectFloat, loadAdditionalWordDelimiters(reader));
        rule.type = Rule::Type::Float;
    } else if (name == QLatin1String("HlCStringChar")) {
        rule.type = Rule::Type::HlCStringChar;
    } else if (name == QLatin1String("RangeDetect")) {
        const auto s1 = attrs.value(QLatin1String("char"));
        const auto s2 = attrs.value(QLatin1String("char1"));
        if (!checkIsChar(s1, "char", defName, reader)) {
            return false;
        }
        if (!checkIsChar(s2, "char1", defName, reader)) {
            return false;
        }

        initRuleData(rule.data.rangeDetect, s1.at(0), s2.at(0));
        rule.type = Rule::Type::RangeDetect;
    } else if (name == QLatin1String("HlCHex")) {
        initRuleData(rule.data.hlCHex, loadAdditionalWordDelimiters(reader));
        rule.type = Rule::Type::HlCHex;
    } else if (name == QLatin1String("HlCChar")) {
        rule.type = Rule::Type::HlCChar;
    } else if (name == QLatin1String("HlCOct")) {
        initRuleData(rule.data.hlCOct, loadAdditionalWordDelimiters(reader));
        rule.type = Rule::Type::HlCOct;
    } else {
        qCWarning(Log) << "Unknown rule type:" << name;
        return false;
    }

    if (!isIncludeRules) {
        rule.common.contextName = attrs.value(QLatin1String("context")).toString();
        rule.common.beginRegionName = attrs.value(QLatin1String("beginRegion")).toString();
        rule.common.endRegionName = attrs.value(QLatin1String("endRegion")).toString();
        rule.common.firstNonSpace = Xml::attrToBool(attrs.value(QLatin1String("firstNonSpace")));
        rule.common.lookAhead = Xml::attrToBool(attrs.value(QLatin1String("lookAhead")));
        // attribute is only used when lookAhead is false
        if (!rule.common.lookAhead) {
            rule.common.attributeName = attrs.value(QLatin1String("attribute")).toString();
        }
        bool colOk = false;
        rule.common.column = attrs.value(QLatin1String("column")).toInt(&colOk);
        if (!colOk) {
            rule.common.column = -1;
        }
    }

    return true;
}

template<class Data1, class Data2, class Visitor>
static void dataRuleVisit(HighlightingContextData::Rule::Type type, Data1 &&data1, Data2 &&data2, Visitor &&visitor)
{
    using Rule = HighlightingContextData::Rule;
    using Type = Rule::Type;
    switch (type) {
    case Type::AnyChar:
        visitor(data1.anyChar, data2.anyChar);
        break;
    case Type::DetectChar:
        visitor(data1.detectChar, data2.detectChar);
        break;
    case Type::Detect2Chars:
        visitor(data1.detect2Chars, data2.detect2Chars);
        break;
    case Type::HlCOct:
        visitor(data1.hlCOct, data2.hlCOct);
        break;
    case Type::IncludeRules:
        visitor(data1.includeRules, data2.includeRules);
        break;
    case Type::Int:
        visitor(data1.detectInt, data2.detectInt);
        break;
    case Type::Keyword:
        visitor(data1.keyword, data2.keyword);
        break;
    case Type::LineContinue:
        visitor(data1.lineContinue, data2.lineContinue);
        break;
    case Type::RangeDetect:
        visitor(data1.rangeDetect, data2.rangeDetect);
        break;
    case Type::RegExpr:
        visitor(data1.regExpr, data2.regExpr);
        break;
    case Type::StringDetect:
        visitor(data1.stringDetect, data2.stringDetect);
        break;
    case Type::WordDetect:
        visitor(data1.wordDetect, data2.wordDetect);
        break;
    case Type::Float:
        visitor(data1.detectFloat, data2.detectFloat);
        break;
    case Type::HlCHex:
        visitor(data1.hlCHex, data2.hlCHex);
        break;

    case Type::HlCStringChar:
    case Type::DetectIdentifier:
    case Type::DetectSpaces:
    case Type::HlCChar:
    case Type::Unknown:;
    }
}

HighlightingContextData::Rule::Rule() noexcept = default;

HighlightingContextData::Rule::Rule(Rule &&other) noexcept
    : common(std::move(other.common))
{
    dataRuleVisit(other.type, data, other.data, [](auto &data1, auto &data2) {
        using Data = std::remove_reference_t<decltype(data1)>;
        new (&data1) Data(std::move(data2));
    });
    type = other.type;
}

HighlightingContextData::Rule::Rule(const Rule &other)
    : common(other.common)
{
    dataRuleVisit(other.type, data, other.data, [](auto &data1, auto &data2) {
        using Data = std::remove_reference_t<decltype(data1)>;
        new (&data1) Data(data2);
    });
    type = other.type;
}

HighlightingContextData::Rule::~Rule()
{
    dataRuleVisit(type, data, data, [](auto &data, auto &) {
        using Data = std::remove_reference_t<decltype(data)>;
        data.~Data();
    });
}

HighlightingContextData::ContextSwitch::ContextSwitch(QStringView str)
{
    if (str.isEmpty() || str == QStringLiteral("#stay")) {
        return;
    }

    while (str.startsWith(QStringLiteral("#pop"))) {
        ++m_popCount;
        if (str.size() > 4 && str.at(4) == QLatin1Char('!')) {
            str = str.mid(5);
            break;
        }
        str = str.mid(4);
    }

    if (str.isEmpty()) {
        return;
    }

    m_contextAndDefName = str.toString();
    m_defNameIndex = str.indexOf(QStringLiteral("##"));
}

bool HighlightingContextData::ContextSwitch::isStay() const
{
    return m_popCount == -1 && m_contextAndDefName.isEmpty();
}

QStringView HighlightingContextData::ContextSwitch::contextName() const
{
    if (m_defNameIndex == -1) {
        return m_contextAndDefName;
    }
    return QStringView(m_contextAndDefName).left(m_defNameIndex);
}

QStringView HighlightingContextData::ContextSwitch::defName() const
{
    if (m_defNameIndex == -1) {
        return QStringView();
    }
    return QStringView(m_contextAndDefName).mid(m_defNameIndex + 2);
}

void HighlightingContextData::load(const QString &defName, QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("context"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    name = reader.attributes().value(QLatin1String("name")).toString();
    attribute = reader.attributes().value(QLatin1String("attribute")).toString();
    lineEndContext = reader.attributes().value(QLatin1String("lineEndContext")).toString();
    lineEmptyContext = reader.attributes().value(QLatin1String("lineEmptyContext")).toString();
    fallthroughContext = reader.attributes().value(QLatin1String("fallthroughContext")).toString();
    noIndentationBasedFolding = Xml::attrToBool(reader.attributes().value(QLatin1String("noIndentationBasedFolding")));

    rules.reserve(8);

    reader.readNext();
    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement: {
            auto &rule = rules.emplace_back();
            if (!loadRule(defName, rule, reader)) {
                rules.pop_back();
            }
            // be done with this rule, skip all subelements, e.g. no longer supported sub-rules
            reader.skipCurrentElement();
            reader.readNext();
            break;
        }
        case QXmlStreamReader::EndElement:
            return;
        default:
            reader.readNext();
            break;
        }
    }
}
