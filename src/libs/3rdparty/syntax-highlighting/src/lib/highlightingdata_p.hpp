/*
    SPDX-FileCopyrightText: 2021 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_HIGHLIGHTING_DATA_P_H
#define KSYNTAXHIGHLIGHTING_HIGHLIGHTING_DATA_P_H

#include <QString>
#include <QStringList>

#include <vector>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
/**
 * Represents the raw xml data of a context and its rules.
 * After resolving contexts, members of this class are no longer
 * use and the instance can be freed to recover used memory.
 */
class HighlightingContextData
{
public:
    void load(const QString &defName, QXmlStreamReader &reader);

    struct ContextSwitch {
        ContextSwitch() = default;
        ContextSwitch(QStringView str);

        QStringView contextName() const;
        QStringView defName() const;

        bool isStay() const;

        int popCount() const
        {
            return m_popCount;
        }

    private:
        int m_popCount = 0;
        int m_defNameIndex = -1;
        QString m_contextAndDefName;
    };

    struct Rule {
        enum class Type : quint8 {
            Unknown,
            AnyChar,
            Detect2Chars,
            DetectChar,
            HlCOct,
            IncludeRules,
            Int,
            Keyword,
            LineContinue,
            RangeDetect,
            RegExpr,
            StringDetect,
            WordDetect,
            Float,
            HlCStringChar,
            DetectIdentifier,
            DetectSpaces,
            HlCChar,
            HlCHex,
        };

        struct AnyChar {
            QString chars;
        };

        struct Detect2Chars {
            QChar char1;
            QChar char2;
        };

        struct DetectChar {
            QChar char1;
            bool dynamic;
        };

        struct WordDelimiters {
            QString additionalDeliminator;
            QString weakDeliminator;
        };

        struct Float {
            WordDelimiters wordDelimiters;
        };

        struct HlCHex {
            WordDelimiters wordDelimiters;
        };

        struct HlCOct {
            WordDelimiters wordDelimiters;
        };

        struct IncludeRules {
            QString contextName;
            bool includeAttribute;
        };

        struct Int {
            WordDelimiters wordDelimiters;
        };

        struct Keyword {
            QString name;
            WordDelimiters wordDelimiters;
            Qt::CaseSensitivity caseSensitivityOverride;
            bool hasCaseSensitivityOverride;
        };

        struct LineContinue {
            QChar char1;
        };

        struct RangeDetect {
            QChar begin;
            QChar end;
        };

        struct RegExpr {
            QString pattern;
            Qt::CaseSensitivity caseSensitivity;
            bool isMinimal;
            bool dynamic;
        };

        struct StringDetect {
            QString string;
            Qt::CaseSensitivity caseSensitivity;
            bool dynamic;
        };

        struct WordDetect {
            QString word;
            WordDelimiters wordDelimiters;
            Qt::CaseSensitivity caseSensitivity;
        };

        union Data {
            AnyChar anyChar;
            Detect2Chars detect2Chars;
            DetectChar detectChar;
            HlCOct hlCOct;
            IncludeRules includeRules;
            Int detectInt;
            Keyword keyword;
            LineContinue lineContinue;
            RangeDetect rangeDetect;
            RegExpr regExpr;
            StringDetect stringDetect;
            WordDetect wordDetect;
            Float detectFloat;
            HlCHex hlCHex;

            Data() noexcept
            {
            }

            ~Data()
            {
            }
        };

        struct Common {
            QString contextName;
            QString attributeName;
            QString beginRegionName;
            QString endRegionName;
            int column = -1;
            bool firstNonSpace = false;
            bool lookAhead = false;
        };

        Type type = Type::Unknown;
        Common common;
        Data data;

        Rule() noexcept;
        Rule(Rule &&other) noexcept;
        Rule(const Rule &other);
        ~Rule();

        // since nothing is deleted in the rules vector, these functions do not need to be implemented
        Rule &operator=(Rule &&other) = delete;
        Rule &operator=(const Rule &other) = delete;
    };

    QString name;

    /**
     * attribute name, to lookup our format
     */
    QString attribute;

    QString lineEndContext;
    QString lineEmptyContext;
    QString fallthroughContext;

    std::vector<Rule> rules;

    bool noIndentationBasedFolding = false;
};
}

#endif
