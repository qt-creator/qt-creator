/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "context_p.h"
#include "definition_p.h"
#include "ksyntaxhighlighting_logging.h"
#include "rule_p.h"
#include "worddelimiters_p.h"
#include "xml_p.h"

#include <QString>

using namespace KSyntaxHighlighting;

// QChar::isDigit() match any digit in unicode (romain numeral, etc)
static bool isDigit(QChar c)
{
    return (c <= QLatin1Char('9') && QLatin1Char('0') <= c);
}

static bool isOctalChar(QChar c)
{
    return (c <= QLatin1Char('7') && QLatin1Char('0') <= c);
}

static bool isHexChar(QChar c)
{
    return isDigit(c) || (c <= QLatin1Char('f') && QLatin1Char('a') <= c) || (c <= QLatin1Char('F') && QLatin1Char('A') <= c);
}

static int matchEscapedChar(QStringView text, int offset)
{
    if (text.at(offset) != QLatin1Char('\\') || text.size() < offset + 2) {
        return offset;
    }

    const auto c = text.at(offset + 1);
    switch (c.unicode()) {
    // control chars
    case 'a':
    case 'b':
    case 'e':
    case 'f':
    case 'n':
    case 'r':
    case 't':
    case 'v':
    case '"':
    case '\'':
    case '?':
    case '\\':
        return offset + 2;

    // hex encoded character
    case 'x':
        if (offset + 2 < text.size() && isHexChar(text.at(offset + 2))) {
            if (offset + 3 < text.size() && isHexChar(text.at(offset + 3))) {
                return offset + 4;
            }
            return offset + 3;
        }
        return offset;

    // octal encoding, simple \0 is OK, too, unlike simple \x above
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
        if (offset + 2 < text.size() && isOctalChar(text.at(offset + 2))) {
            if (offset + 3 < text.size() && isOctalChar(text.at(offset + 3))) {
                return offset + 4;
            }
            return offset + 3;
        }
        return offset + 2;
    }

    return offset;
}

static QString replaceCaptures(const QString &pattern, const QStringList &captures, bool quote)
{
    auto result = pattern;
    for (int i = captures.size() - 1; i >= 1; --i) {
        result.replace(QLatin1Char('%') + QString::number(i), quote ? QRegularExpression::escape(captures.at(i)) : captures.at(i));
    }
    return result;
}

static MatchResult matchString(QStringView pattern, QStringView text, int offset, Qt::CaseSensitivity caseSensitivity)
{
    if (offset + pattern.size() <= text.size() && text.mid(offset, pattern.size()).compare(pattern, caseSensitivity) == 0) {
        return offset + pattern.size();
    }
    return offset;
}

static void resolveAdditionalWordDelimiters(WordDelimiters &wordDelimiters, const HighlightingContextData::Rule::WordDelimiters &delimiters)
{
    // cache for DefinitionData::wordDelimiters, is accessed VERY often
    if (!delimiters.additionalDeliminator.isEmpty() || !delimiters.weakDeliminator.isEmpty()) {
        wordDelimiters.append(QStringView(delimiters.additionalDeliminator));
        wordDelimiters.remove(QStringView(delimiters.weakDeliminator));
    }
}

Rule::~Rule() = default;

const IncludeRules *Rule::castToIncludeRules() const
{
    if (m_type != Type::IncludeRules) {
        return nullptr;
    }
    return static_cast<const IncludeRules *>(this);
}

bool Rule::resolveCommon(DefinitionData &def, const HighlightingContextData::Rule &ruleData, QStringView lookupContextName)
{
    switch (ruleData.type) {
    // IncludeRules uses this with a different semantic
    case HighlightingContextData::Rule::Type::IncludeRules:
        m_type = Type::IncludeRules;
        return true;
    case HighlightingContextData::Rule::Type::LineContinue:
        m_type = Type::LineContinue;
        break;
    default:
        m_type = Type::OtherRule;
        break;
    }

    /**
     * try to get our format from the definition we stem from
     */
    if (!ruleData.common.attributeName.isEmpty()) {
        m_attributeFormat = def.formatByName(ruleData.common.attributeName);
        if (!m_attributeFormat.isValid()) {
            qCWarning(Log) << "Rule: Unknown format" << ruleData.common.attributeName << "in context" << lookupContextName << "of definition" << def.name;
        }
    }

    m_firstNonSpace = ruleData.common.firstNonSpace;
    m_lookAhead = ruleData.common.lookAhead;
    m_column = ruleData.common.column;

    if (!ruleData.common.beginRegionName.isEmpty()) {
        m_beginRegion = FoldingRegion(FoldingRegion::Begin, def.foldingRegionId(ruleData.common.beginRegionName));
    }
    if (!ruleData.common.endRegionName.isEmpty()) {
        m_endRegion = FoldingRegion(FoldingRegion::End, def.foldingRegionId(ruleData.common.endRegionName));
    }

    m_context.resolve(def, ruleData.common.contextName);

    return !(m_lookAhead && m_context.isStay());
}

static Rule::Ptr createRule(DefinitionData &def, const HighlightingContextData::Rule &ruleData, QStringView lookupContextName)
{
    using Type = HighlightingContextData::Rule::Type;

    switch (ruleData.type) {
    case Type::AnyChar:
        return std::make_shared<AnyChar>(ruleData.data.anyChar);
    case Type::DetectChar:
        return std::make_shared<DetectChar>(ruleData.data.detectChar);
    case Type::Detect2Chars:
        return std::make_shared<Detect2Chars>(ruleData.data.detect2Chars);
    case Type::IncludeRules:
        return std::make_shared<IncludeRules>(ruleData.data.includeRules);
    case Type::Int:
        return std::make_shared<Int>(def, ruleData.data.detectInt);
    case Type::Keyword:
        return KeywordListRule::create(def, ruleData.data.keyword, lookupContextName);
    case Type::LineContinue:
        return std::make_shared<LineContinue>(ruleData.data.lineContinue);
    case Type::RangeDetect:
        return std::make_shared<RangeDetect>(ruleData.data.rangeDetect);
    case Type::RegExpr:
        if (!ruleData.data.regExpr.dynamic) {
            return std::make_shared<RegExpr>(ruleData.data.regExpr);
        } else {
            return std::make_shared<DynamicRegExpr>(ruleData.data.regExpr);
        }
    case Type::StringDetect:
        if (ruleData.data.stringDetect.dynamic) {
            return std::make_shared<DynamicStringDetect>(ruleData.data.stringDetect);
        }
        return std::make_shared<StringDetect>(ruleData.data.stringDetect);
    case Type::WordDetect:
        return std::make_shared<WordDetect>(def, ruleData.data.wordDetect);
    case Type::Float:
        return std::make_shared<Float>(def, ruleData.data.detectFloat);
    case Type::HlCOct:
        return std::make_shared<HlCOct>(def, ruleData.data.hlCOct);
    case Type::HlCStringChar:
        return std::make_shared<HlCStringChar>();
    case Type::DetectIdentifier:
        return std::make_shared<DetectIdentifier>();
    case Type::DetectSpaces:
        return std::make_shared<DetectSpaces>();
    case Type::HlCChar:
        return std::make_shared<HlCChar>();
    case Type::HlCHex:
        return std::make_shared<HlCHex>(def, ruleData.data.hlCHex);

    case Type::Unknown:;
    }

    return Rule::Ptr(nullptr);
}

Rule::Ptr Rule::create(DefinitionData &def, const HighlightingContextData::Rule &ruleData, QStringView lookupContextName)
{
    auto rule = createRule(def, ruleData, lookupContextName);
    if (rule && !rule->resolveCommon(def, ruleData, lookupContextName)) {
        rule.reset();
    }
    return rule;
}

AnyChar::AnyChar(const HighlightingContextData::Rule::AnyChar &data)
    : m_chars(data.chars)
{
}

MatchResult AnyChar::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (m_chars.contains(text.at(offset))) {
        return offset + 1;
    }
    return offset;
}

DetectChar::DetectChar(const HighlightingContextData::Rule::DetectChar &data)
    : m_char(data.char1)
    , m_captureIndex(data.dynamic ? data.char1.digitValue() : 0)
{
    m_dynamic = data.dynamic;
}

MatchResult DetectChar::doMatch(QStringView text, int offset, const QStringList &captures) const
{
    if (m_dynamic) {
        if (m_captureIndex == 0 || captures.size() <= m_captureIndex || captures.at(m_captureIndex).isEmpty()) {
            return offset;
        }
        if (text.at(offset) == captures.at(m_captureIndex).at(0)) {
            return offset + 1;
        }
        return offset;
    }

    if (text.at(offset) == m_char) {
        return offset + 1;
    }
    return offset;
}

Detect2Chars::Detect2Chars(const HighlightingContextData::Rule::Detect2Chars &data)
    : m_char1(data.char1)
    , m_char2(data.char2)
{
}

MatchResult Detect2Chars::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (text.size() - offset < 2) {
        return offset;
    }
    if (text.at(offset) == m_char1 && text.at(offset + 1) == m_char2) {
        return offset + 2;
    }
    return offset;
}

MatchResult DetectIdentifier::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (!text.at(offset).isLetter() && text.at(offset) != QLatin1Char('_')) {
        return offset;
    }

    for (int i = offset + 1; i < text.size(); ++i) {
        const auto c = text.at(i);
        if (!c.isLetterOrNumber() && c != QLatin1Char('_')) {
            return i;
        }
    }

    return text.size();
}

MatchResult DetectSpaces::doMatch(QStringView text, int offset, const QStringList &) const
{
    while (offset < text.size() && text.at(offset).isSpace()) {
        ++offset;
    }
    return offset;
}

Float::Float(DefinitionData &def, const HighlightingContextData::Rule::Float &data)
    : m_wordDelimiters(def.wordDelimiters)
{
    resolveAdditionalWordDelimiters(m_wordDelimiters, data.wordDelimiters);
}

MatchResult Float::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (offset > 0 && !m_wordDelimiters.contains(text.at(offset - 1))) {
        return offset;
    }

    auto newOffset = offset;
    while (newOffset < text.size() && isDigit(text.at(newOffset))) {
        ++newOffset;
    }

    if (newOffset >= text.size() || text.at(newOffset) != QLatin1Char('.')) {
        return offset;
    }
    ++newOffset;

    while (newOffset < text.size() && isDigit(text.at(newOffset))) {
        ++newOffset;
    }

    if (newOffset == offset + 1) { // we only found a decimal point
        return offset;
    }

    auto expOffset = newOffset;
    if (expOffset >= text.size() || (text.at(expOffset) != QLatin1Char('e') && text.at(expOffset) != QLatin1Char('E'))) {
        return newOffset;
    }
    ++expOffset;

    if (expOffset < text.size() && (text.at(expOffset) == QLatin1Char('+') || text.at(expOffset) == QLatin1Char('-'))) {
        ++expOffset;
    }
    bool foundExpDigit = false;
    while (expOffset < text.size() && isDigit(text.at(expOffset))) {
        ++expOffset;
        foundExpDigit = true;
    }

    if (!foundExpDigit) {
        return newOffset;
    }
    return expOffset;
}

MatchResult HlCChar::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (text.size() < offset + 3) {
        return offset;
    }

    if (text.at(offset) != QLatin1Char('\'') || text.at(offset + 1) == QLatin1Char('\'')) {
        return offset;
    }

    auto newOffset = matchEscapedChar(text, offset + 1);
    if (newOffset == offset + 1) {
        if (text.at(newOffset) == QLatin1Char('\\')) {
            return offset;
        } else {
            ++newOffset;
        }
    }
    if (newOffset >= text.size()) {
        return offset;
    }

    if (text.at(newOffset) == QLatin1Char('\'')) {
        return newOffset + 1;
    }

    return offset;
}

HlCHex::HlCHex(DefinitionData &def, const HighlightingContextData::Rule::HlCHex &data)
    : m_wordDelimiters(def.wordDelimiters)
{
    resolveAdditionalWordDelimiters(m_wordDelimiters, data.wordDelimiters);
}

MatchResult HlCHex::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (offset > 0 && !m_wordDelimiters.contains(text.at(offset - 1))) {
        return offset;
    }

    if (text.size() < offset + 3) {
        return offset;
    }

    if (text.at(offset) != QLatin1Char('0') || (text.at(offset + 1) != QLatin1Char('x') && text.at(offset + 1) != QLatin1Char('X'))) {
        return offset;
    }

    if (!isHexChar(text.at(offset + 2))) {
        return offset;
    }

    offset += 3;
    while (offset < text.size() && isHexChar(text.at(offset))) {
        ++offset;
    }

    // TODO Kate matches U/L suffix, QtC does not?

    return offset;
}

HlCOct::HlCOct(DefinitionData &def, const HighlightingContextData::Rule::HlCOct &data)
    : m_wordDelimiters(def.wordDelimiters)
{
    resolveAdditionalWordDelimiters(m_wordDelimiters, data.wordDelimiters);
}

MatchResult HlCOct::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (offset > 0 && !m_wordDelimiters.contains(text.at(offset - 1))) {
        return offset;
    }

    if (text.size() < offset + 2) {
        return offset;
    }

    if (text.at(offset) != QLatin1Char('0')) {
        return offset;
    }

    if (!isOctalChar(text.at(offset + 1))) {
        return offset;
    }

    offset += 2;
    while (offset < text.size() && isOctalChar(text.at(offset))) {
        ++offset;
    }

    return offset;
}

MatchResult HlCStringChar::doMatch(QStringView text, int offset, const QStringList &) const
{
    return matchEscapedChar(text, offset);
}

IncludeRules::IncludeRules(const HighlightingContextData::Rule::IncludeRules &data)
    : m_contextName(data.contextName)
    , m_includeAttribute(data.includeAttribute)
{
}

MatchResult IncludeRules::doMatch(QStringView text, int offset, const QStringList &) const
{
    Q_UNUSED(text);
    qCWarning(Log) << "Unresolved include rule";
    return offset;
}

Int::Int(DefinitionData &def, const HighlightingContextData::Rule::Int &data)
    : m_wordDelimiters(def.wordDelimiters)
{
    resolveAdditionalWordDelimiters(m_wordDelimiters, data.wordDelimiters);
}

MatchResult Int::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (offset > 0 && !m_wordDelimiters.contains(text.at(offset - 1))) {
        return offset;
    }

    while (offset < text.size() && isDigit(text.at(offset))) {
        ++offset;
    }
    return offset;
}

Rule::Ptr KeywordListRule::create(DefinitionData &def, const HighlightingContextData::Rule::Keyword &data, QStringView lookupContextName)
{
    /**
     * get our keyword list, if not found => bail out
     */
    auto *keywordList = def.keywordList(data.name);
    if (!keywordList) {
        qCWarning(Log) << "Rule: Unknown keyword list" << data.name << "in context" << lookupContextName << "of definition" << def.name;
        return Rule::Ptr();
    }

    if (keywordList->isEmpty()) {
        return Rule::Ptr();
    }

    /**
     * we might overwrite the case sensitivity
     * then we need to init the list for lookup of that sensitivity setting
     */
    if (data.hasCaseSensitivityOverride) {
        keywordList->initLookupForCaseSensitivity(data.caseSensitivityOverride);
    }

    return std::make_shared<KeywordListRule>(*keywordList, def, data);
}

KeywordListRule::KeywordListRule(const KeywordList &keywordList, DefinitionData &def, const HighlightingContextData::Rule::Keyword &data)
    : m_wordDelimiters(def.wordDelimiters)
    , m_keywordList(keywordList)
    , m_caseSensitivity(data.hasCaseSensitivityOverride ? data.caseSensitivityOverride : keywordList.caseSensitivity())
{
    resolveAdditionalWordDelimiters(m_wordDelimiters, data.wordDelimiters);
}

MatchResult KeywordListRule::doMatch(QStringView text, int offset, const QStringList &) const
{
    auto newOffset = offset;
    while (text.size() > newOffset && !m_wordDelimiters.contains(text.at(newOffset))) {
        ++newOffset;
    }
    if (newOffset == offset) {
        return offset;
    }

    if (m_keywordList.contains(text.mid(offset, newOffset - offset), m_caseSensitivity)) {
        return newOffset;
    }

    // we don't match, but we can skip until newOffset as we can't start a keyword in-between
    return MatchResult(offset, newOffset);
}

LineContinue::LineContinue(const HighlightingContextData::Rule::LineContinue &data)
    : m_char(data.char1)
{
}

MatchResult LineContinue::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (offset == text.size() - 1 && text.at(offset) == m_char) {
        return offset + 1;
    }
    return offset;
}

RangeDetect::RangeDetect(const HighlightingContextData::Rule::RangeDetect &data)
    : m_begin(data.begin)
    , m_end(data.end)
{
}

MatchResult RangeDetect::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (text.size() - offset < 2) {
        return offset;
    }
    if (text.at(offset) != m_begin) {
        return offset;
    }

    auto newOffset = offset + 1;
    while (newOffset < text.size()) {
        if (text.at(newOffset) == m_end) {
            return newOffset + 1;
        }
        ++newOffset;
    }
    return offset;
}

static QRegularExpression::PatternOptions makePattenOptions(const HighlightingContextData::Rule::RegExpr &data)
{
    return (data.isMinimal ? QRegularExpression::InvertedGreedinessOption : QRegularExpression::NoPatternOption)
        | (data.caseSensitivity == Qt::CaseInsensitive ? QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption)
        // DontCaptureOption is removed by resolve() when necessary
        | QRegularExpression::DontCaptureOption
        // ensure Unicode support is enabled
        | QRegularExpression::UseUnicodePropertiesOption;
}

static void resolveRegex(QRegularExpression &regexp, Context *context)
{
    if (!regexp.isValid()) {
        // DontCaptureOption with back reference capture is an error, remove this option then try again
        regexp.setPatternOptions(regexp.patternOptions() & ~QRegularExpression::DontCaptureOption);

        if (!regexp.isValid()) {
            qCDebug(Log) << "Invalid regexp:" << regexp.pattern();
        }

        return;
    }

    // disable DontCaptureOption when reference a context with dynamic rule
    if (context) {
        for (const Rule::Ptr &rule : context->rules()) {
            if (rule->isDynamic()) {
                regexp.setPatternOptions(regexp.patternOptions() & ~QRegularExpression::DontCaptureOption);
                break;
            }
        }
    }
}

static MatchResult regexMatch(const QRegularExpression &regexp, QStringView text, int offset)
{
    /**
     * match the pattern
     */
    const auto result = regexp.match(text, offset, QRegularExpression::NormalMatch, QRegularExpression::DontCheckSubjectStringMatchOption);
    if (result.capturedStart() == offset) {
        /**
         * we only need to compute the captured texts if we have real capture groups
         * highlightings should only address %1..%.., see e.g. replaceCaptures
         * DetectChar ignores %0, too
         */
        if (result.lastCapturedIndex() > 0) {
            return MatchResult(offset + result.capturedLength(), result.capturedTexts());
        }

        /**
         * else: ignore the implicit 0 group we always capture, no need to allocate stuff for that
         */
        return MatchResult(offset + result.capturedLength());
    }

    /**
     * no match
     * we can always compute the skip offset as the highlighter will invalidate the cache for changed captures for dynamic rules!
     */
    return MatchResult(offset, result.capturedStart());
}

RegExpr::RegExpr(const HighlightingContextData::Rule::RegExpr &data)
    : m_regexp(data.pattern, makePattenOptions(data))
{
}

void RegExpr::resolve()
{
    if (m_isResolved) {
        return;
    }

    m_isResolved = true;

    resolveRegex(m_regexp, context().context());
}

MatchResult RegExpr::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (Q_UNLIKELY(!m_isResolved)) {
        const_cast<RegExpr *>(this)->resolve();
    }

    return regexMatch(m_regexp, text, offset);
}

DynamicRegExpr::DynamicRegExpr(const HighlightingContextData::Rule::RegExpr &data)
    : m_pattern(data.pattern)
    , m_patternOptions(makePattenOptions(data))
{
    m_dynamic = true;
}

void DynamicRegExpr::resolve()
{
    if (m_isResolved) {
        return;
    }

    m_isResolved = true;

    QRegularExpression regexp(m_pattern, m_patternOptions);
    resolveRegex(regexp, context().context());
    m_patternOptions = regexp.patternOptions();
}

MatchResult DynamicRegExpr::doMatch(QStringView text, int offset, const QStringList &captures) const
{
    if (Q_UNLIKELY(!m_isResolved)) {
        const_cast<DynamicRegExpr *>(this)->resolve();
    }

    /**
     * create new pattern with right instantiation
     */
    const QRegularExpression regexp(replaceCaptures(m_pattern, captures, true), m_patternOptions);

    return regexMatch(regexp, text, offset);
}

StringDetect::StringDetect(const HighlightingContextData::Rule::StringDetect &data)
    : m_string(data.string)
    , m_caseSensitivity(data.caseSensitivity)
{
}

MatchResult StringDetect::doMatch(QStringView text, int offset, const QStringList &) const
{
    return matchString(m_string, text, offset, m_caseSensitivity);
}

DynamicStringDetect::DynamicStringDetect(const HighlightingContextData::Rule::StringDetect &data)
    : m_string(data.string)
    , m_caseSensitivity(data.caseSensitivity)
{
    m_dynamic = true;
}

MatchResult DynamicStringDetect::doMatch(QStringView text, int offset, const QStringList &captures) const
{
    /**
     * for dynamic case: create new pattern with right instantiation
     */
    const auto pattern = replaceCaptures(m_string, captures, false);
    return matchString(pattern, text, offset, m_caseSensitivity);
}

WordDetect::WordDetect(DefinitionData &def, const HighlightingContextData::Rule::WordDetect &data)
    : m_wordDelimiters(def.wordDelimiters)
    , m_word(data.word)
    , m_caseSensitivity(data.caseSensitivity)
{
    resolveAdditionalWordDelimiters(m_wordDelimiters, data.wordDelimiters);
}

MatchResult WordDetect::doMatch(QStringView text, int offset, const QStringList &) const
{
    if (text.size() - offset < m_word.size()) {
        return offset;
    }

    /**
     * detect delimiter characters on the inner and outer boundaries of the string
     * NOTE: m_word isn't empty
     */
    if (offset > 0 && !m_wordDelimiters.contains(text.at(offset - 1)) && !m_wordDelimiters.contains(text.at(offset))) {
        return offset;
    }

    if (text.mid(offset, m_word.size()).compare(m_word, m_caseSensitivity) != 0) {
        return offset;
    }

    if (text.size() == offset + m_word.size() || m_wordDelimiters.contains(text.at(offset + m_word.size()))
        || m_wordDelimiters.contains(text.at(offset + m_word.size() - 1))) {
        return offset + m_word.size();
    }

    return offset;
}
