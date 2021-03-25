/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_KEYWORDLIST_P_H
#define KSYNTAXHIGHLIGHTING_KEYWORDLIST_P_H

#include <QString>
#include <QStringList>
#include <QStringView>

#include <vector>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
class Repository;
class DefinitionData;

class KeywordList
{
public:
    KeywordList() = default;
    ~KeywordList() = default;

    bool isEmpty() const
    {
        return m_keywords.isEmpty();
    }

    const QString &name() const
    {
        return m_name;
    }

    const QStringList &keywords() const
    {
        return m_keywords;
    }

    void setKeywordList(const QStringList &keywords)
    {
        m_keywords = keywords;
        m_keywordsSortedCaseSensitive.clear();
        m_keywordsSortedCaseInsensitive.clear();
        initLookupForCaseSensitivity(m_caseSensitive);
    }

    /** Checks if @p str is a keyword in this list. */
    bool contains(QStringView str) const { return contains(str, m_caseSensitive); }

    /** Checks if @p str is a keyword in this list, overriding the global case-sensitivity setting. */
    bool contains(QStringView str, Qt::CaseSensitivity caseSensitive) const;

    void load(QXmlStreamReader &reader);
    void setCaseSensitivity(Qt::CaseSensitivity caseSensitive);
    void initLookupForCaseSensitivity(Qt::CaseSensitivity caseSensitive);
    void resolveIncludeKeywords(DefinitionData &def);

private:
    /**
     * name of keyword list as in XML
     */
    QString m_name;

    /**
     * raw list of keywords, as seen in XML (but trimmed)
     */
    QStringList m_keywords;

    /**
     * raw list of include keywords, as seen in XML (but trimmed)
     */
    QStringList m_includes;

    /**
     * default case-sensitivity setting
     */
    Qt::CaseSensitivity m_caseSensitive = Qt::CaseSensitive;

    /**
     * case-sensitive sorted string references to m_keywords for lookup
     */
    std::vector<QStringView> m_keywordsSortedCaseSensitive;

    /**
     * case-insensitive sorted string references to m_keywords for lookup
     */
    std::vector<QStringView> m_keywordsSortedCaseInsensitive;
};
}

#endif // KSYNTAXHIGHLIGHTING_KEYWORDLIST_P_H
