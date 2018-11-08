/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
#include <QVector>

#include <memory>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting {

class Rule
{
public:
    Rule() = default;
    virtual ~Rule() = default;

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

    virtual MatchResult doMatch(const QString &text, int offset, const QStringList &captures) const = 0;

    static Rule::Ptr create(const QStringRef &name);

protected:
    virtual bool doLoad(QXmlStreamReader &reader);

    bool isWordDelimiter(QChar c) const;

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
    QStringRef m_wordDelimiter;
};


class AnyChar : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;

private:
    QString m_chars;
};

class DetectChar : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList &captures) const override;

private:
    QChar m_char;
    bool m_dynamic = false;
    int m_captureIndex = 0;
};

class Detect2Char : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList &captures) const override;

private:
    QChar m_char1;
    QChar m_char2;
};

class DetectIdentifier : public Rule
{
protected:
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;
};

class DetectSpaces : public Rule
{
protected:
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;
};

class Float : public Rule
{
protected:
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;
};

class IncludeRules : public Rule
{
public:
    QString contextName() const;
    QString definitionName() const;
    bool includeAttribute() const;

protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;

private:
    QString m_contextName;
    QString m_defName;
    bool m_includeAttribute;
};

class Int : public Rule
{
protected:
    MatchResult doMatch(const QString & text, int offset, const QStringList &captures) const override;
};

class HlCChar : public Rule
{
protected:
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;
};

class HlCHex : public Rule
{
protected:
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;
};

class HlCOct : public Rule
{
protected:
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;
};

class HlCStringChar : public Rule
{
protected:
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;
};

class KeywordListRule : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;

private:
    KeywordList *m_keywordList;
    bool m_hasCaseSensitivityOverride;
    Qt::CaseSensitivity m_caseSensitivityOverride;
};

class LineContinue : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;

private:
    QChar m_char;
};

class RangeDetect : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList&) const override;

private:
    QChar m_begin;
    QChar m_end;
};

class RegExpr : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList &captures) const override;

private:
    QRegularExpression m_regexp;
    bool m_dynamic = false;
};

class StringDetect : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList &captures) const override;

private:
    QString m_string;
    Qt::CaseSensitivity m_caseSensitivity;
    bool m_dynamic = false;
};

class WordDetect : public Rule
{
protected:
    bool doLoad(QXmlStreamReader & reader) override;
    MatchResult doMatch(const QString & text, int offset, const QStringList &captures) const override;

private:
    QString m_word;
    Qt::CaseSensitivity m_caseSensitivity;
};

}

#endif // KSYNTAXHIGHLIGHTING_RULE_P_H
