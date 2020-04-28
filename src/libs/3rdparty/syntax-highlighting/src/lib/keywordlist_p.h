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

#ifndef KSYNTAXHIGHLIGHTING_KEYWORDLIST_P_H
#define KSYNTAXHIGHLIGHTING_KEYWORDLIST_P_H

#include <QString>
#include <QStringList>

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
    bool contains(const QStringRef &str) const
    {
        return contains(str, m_caseSensitive);
    }

    /** Checks if @p str is a keyword in this list, overriding the global case-sensitivity setting. */
    bool contains(const QStringRef &str, Qt::CaseSensitivity caseSensitive) const;

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
    std::vector<QStringRef> m_keywordsSortedCaseSensitive;

    /**
     * case-insensitive sorted string references to m_keywords for lookup
     */
    std::vector<QStringRef> m_keywordsSortedCaseInsensitive;
};
}

#endif // KSYNTAXHIGHLIGHTING_KEYWORDLIST_P_H
