/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_RULE_P_H
#define KSYNTAXHIGHLIGHTING_RULE_P_H

#include "contextswitch_p.h"
#include "definitionref_p.h"
#include "foldingregion.h"
#include "format.h"
#include "highlightingdata_p.hpp"
#include "keywordlist_p.h"
#include "matchresult_p.h"
#include "worddelimiters_p.h"

#include <QRegularExpression>
#include <QString>

#include <memory>

namespace KSyntaxHighlighting
{
class WordDelimiters;
class DefinitionData;
class IncludeRules;

class Rule
{
public:
    Rule() = default;
    virtual ~Rule();

    typedef std::shared_ptr<Rule> Ptr;

    const Format &attributeFormat() const
    {
        return m_attributeFormat;
    }

    const ContextSwitch &context() const
    {
        return m_context;
    }

    bool isLookAhead() const
    {
        return m_lookAhead;
    }

    bool isDynamic() const
    {
        return m_dynamic;
    }

    bool firstNonSpace() const
    {
        return m_firstNonSpace;
    }

    int requiredColumn() const
    {
        return m_column;
    }

    const FoldingRegion &beginRegion() const
    {
        return m_beginRegion;
    }

    const FoldingRegion &endRegion() const
    {
        return m_endRegion;
    }

    const IncludeRules *castToIncludeRules() const;

    bool isLineContinue() const
    {
        return m_type == Type::LineContinue;
    }

    virtual MatchResult doMatch(QStringView text, int offset, const QStringList &captures) const = 0;

    static Rule::Ptr create(DefinitionData &def, const HighlightingContextData::Rule &ruleData, QStringView lookupContextName);

private:
    Q_DISABLE_COPY(Rule)

    bool resolveCommon(DefinitionData &def, const HighlightingContextData::Rule &ruleData, QStringView lookupContextName);

    enum class Type : quint8 {
        OtherRule,
        LineContinue,
        IncludeRules,
    };

    Format m_attributeFormat;
    ContextSwitch m_context;
    int m_column = -1;
    FoldingRegion m_beginRegion;
    FoldingRegion m_endRegion;
    Type m_type;
    bool m_firstNonSpace = false;
    bool m_lookAhead = false;

protected:
    bool m_dynamic = false;
};

class AnyChar final : public Rule
{
public:
    AnyChar(const HighlightingContextData::Rule::AnyChar &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    QString m_chars;
};

class DetectChar final : public Rule
{
public:
    DetectChar(const HighlightingContextData::Rule::DetectChar &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    QChar m_char;
    int m_captureIndex = 0;
};

class Detect2Chars final : public Rule
{
public:
    Detect2Chars(const HighlightingContextData::Rule::Detect2Chars &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    QChar m_char1;
    QChar m_char2;
};

class DetectIdentifier final : public Rule
{
protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;
};

class DetectSpaces final : public Rule
{
protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;
};

class Float final : public Rule
{
public:
    Float(DefinitionData &def, const HighlightingContextData::Rule::Float &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    WordDelimiters m_wordDelimiters;
};

class IncludeRules final : public Rule
{
public:
    IncludeRules(const HighlightingContextData::Rule::IncludeRules &data);

    const QString &contextName() const
    {
        return m_contextName;
    }

    bool includeAttribute() const
    {
        return m_includeAttribute;
    }

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    QString m_contextName;
    bool m_includeAttribute;
};

class Int final : public Rule
{
public:
    Int(DefinitionData &def, const HighlightingContextData::Rule::Int &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    WordDelimiters m_wordDelimiters;
};

class HlCChar final : public Rule
{
protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;
};

class HlCHex final : public Rule
{
public:
    HlCHex(DefinitionData &def, const HighlightingContextData::Rule::HlCHex &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    WordDelimiters m_wordDelimiters;
};

class HlCOct final : public Rule
{
public:
    HlCOct(DefinitionData &def, const HighlightingContextData::Rule::HlCOct &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    WordDelimiters m_wordDelimiters;
};

class HlCStringChar final : public Rule
{
protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;
};

class KeywordListRule final : public Rule
{
public:
    KeywordListRule(const KeywordList &keywordList, DefinitionData &def, const HighlightingContextData::Rule::Keyword &data);

    static Rule::Ptr create(DefinitionData &def, const HighlightingContextData::Rule::Keyword &data, QStringView lookupContextName);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    WordDelimiters m_wordDelimiters;
    const KeywordList &m_keywordList;
    Qt::CaseSensitivity m_caseSensitivity;
};

class LineContinue final : public Rule
{
public:
    LineContinue(const HighlightingContextData::Rule::LineContinue &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    QChar m_char;
};

class RangeDetect final : public Rule
{
public:
    RangeDetect(const HighlightingContextData::Rule::RangeDetect &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    QChar m_begin;
    QChar m_end;
};

class RegExpr final : public Rule
{
public:
    RegExpr(const HighlightingContextData::Rule::RegExpr &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    void resolve();
    QRegularExpression m_regexp;
    bool m_isResolved = false;
};

class DynamicRegExpr final : public Rule
{
public:
    DynamicRegExpr(const HighlightingContextData::Rule::RegExpr &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    void resolve();
    QString m_pattern;
    QRegularExpression::PatternOptions m_patternOptions;
    bool m_isResolved = false;
};

class StringDetect final : public Rule
{
public:
    StringDetect(const HighlightingContextData::Rule::StringDetect &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    QString m_string;
    Qt::CaseSensitivity m_caseSensitivity;
};

class DynamicStringDetect final : public Rule
{
public:
    DynamicStringDetect(const HighlightingContextData::Rule::StringDetect &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    QString m_string;
    Qt::CaseSensitivity m_caseSensitivity;
};

class WordDetect final : public Rule
{
public:
    WordDetect(DefinitionData &def, const HighlightingContextData::Rule::WordDetect &data);

protected:
    MatchResult doMatch(QStringView text, int offset, const QStringList &) const override;

private:
    WordDelimiters m_wordDelimiters;
    QString m_word;
    Qt::CaseSensitivity m_caseSensitivity;
};

}

#endif // KSYNTAXHIGHLIGHTING_RULE_P_H
