/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_RULE_P_H
#define KSYNTAXHIGHLIGHTING_RULE_P_H

#include "contextswitch_p.h"
#include "definition.h"
#include "definitionref_p.h"
#include "foldingregion.h"
#include "format.h"
#include "keywordlist_p.h"
#include "matchresult_p.h"

#include <QRegularExpression>
#include <QString>

#include <memory>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
class WordDelimiters;

class Rule
{
public:
    Rule() = default;
    virtual ~Rule();

    typedef std::shared_ptr<Rule> Ptr;

    Definition definition() const;
    void setDefinition(const Definition &def);

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

    bool load(QXmlStreamReader &reader);
    void resolveContext();
    void resolveAttributeFormat(Context *lookupContext);
    virtual void resolvePostProcessing()
    {
    }

    virtual MatchResult doMatch(const QString &text, int offset, const QStringList &captures) const = 0;

    static Rule::Ptr create(QStringView name);

protected:
    virtual bool doLoad(QXmlStreamReader &reader);

    bool isWordDelimiter(QChar c) const;

    void loadAdditionalWordDelimiters(QXmlStreamReader &reader);

private:
    Q_DISABLE_COPY(Rule)

    DefinitionRef m_def;
    QString m_attribute;
    Format m_attributeFormat;
    ContextSwitch m_context;
    int m_column = -1;
    FoldingRegion m_beginRegion;
    FoldingRegion m_endRegion;
    bool m_firstNonSpace = false;
    bool m_lookAhead = false;

    // cache for DefinitionData::wordDelimiters, is accessed VERY often
    WordDelimiters *m_wordDelimiters = nullptr;

    QString m_additionalDeliminator;
    QString m_weakDeliminator;

protected:
    bool m_dynamic = false;
};

class AnyChar : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;

private:
    QString m_chars;
};

class DetectChar : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &captures) const override;

private:
    QChar m_char;
    int m_captureIndex = 0;
};

class Detect2Char : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &captures) const override;

private:
    QChar m_char1;
    QChar m_char2;
};

class DetectIdentifier : public Rule
{
protected:
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;
};

class DetectSpaces : public Rule
{
protected:
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;
};

class Float : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;
};

class IncludeRules : public Rule
{
public:
    QString contextName() const;
    QString definitionName() const;
    bool includeAttribute() const;

protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;

private:
    QString m_contextName;
    QString m_defName;
    bool m_includeAttribute;
};

class Int : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &captures) const override;
};

class HlCChar : public Rule
{
protected:
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;
};

class HlCHex : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;
};

class HlCOct : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;
};

class HlCStringChar : public Rule
{
protected:
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;
};

class KeywordListRule : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;

private:
    KeywordList *m_keywordList;
    bool m_hasCaseSensitivityOverride;
    Qt::CaseSensitivity m_caseSensitivityOverride;
};

class LineContinue : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;

private:
    QChar m_char;
};

class RangeDetect : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &) const override;

private:
    QChar m_begin;
    QChar m_end;
};

class RegExpr : public Rule
{
protected:
    void resolvePostProcessing() override;
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &captures) const override;

private:
    QRegularExpression m_regexp;
    bool m_isResolved = false;
};

class StringDetect : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &captures) const override;

private:
    QString m_string;
    Qt::CaseSensitivity m_caseSensitivity;
};

class WordDetect : public Rule
{
protected:
    bool doLoad(QXmlStreamReader &reader) override;
    MatchResult doMatch(const QString &text, int offset, const QStringList &captures) const override;

private:
    QString m_word;
    Qt::CaseSensitivity m_caseSensitivity;
};

}

#endif // KSYNTAXHIGHLIGHTING_RULE_P_H
