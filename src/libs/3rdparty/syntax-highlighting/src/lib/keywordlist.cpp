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

#include "keywordlist_p.h"

#include <QDebug>
#include <QXmlStreamReader>

#include <algorithm>

using namespace KSyntaxHighlighting;

bool KeywordList::contains(const QStringRef &str, Qt::CaseSensitivity caseSensitive) const
{
    /**
     * get right vector to search in
     */
    const auto &vectorToSearch = (caseSensitive == Qt::CaseSensitive) ? m_keywordsSortedCaseSensitive : m_keywordsSortedCaseInsensitive;

    /**
     * search with right predicate
     */
    return std::binary_search(vectorToSearch.begin(), vectorToSearch.end(), str, [caseSensitive] (const QStringRef &a, const QStringRef &b) { return a.compare(b, caseSensitive) < 0; });
}

void KeywordList::load(QXmlStreamReader& reader)
{
    Q_ASSERT(reader.name() == QLatin1String("list"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    m_name = reader.attributes().value(QStringLiteral("name")).toString();

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
            case QXmlStreamReader::StartElement:
                if (reader.name() == QLatin1String("item")) {
                    m_keywords.append(reader.readElementText().trimmed());
                    reader.readNextStartElement();
                    break;
                }
                reader.readNext();
                break;
            case QXmlStreamReader::EndElement:
                reader.readNext();
                return;
            default:
                reader.readNext();
                break;
        }
    }
}

void KeywordList::setCaseSensitivity(Qt::CaseSensitivity caseSensitive)
{
    /**
     * remember default case-sensitivity and init lookup for it
     */
    m_caseSensitive = caseSensitive;
    initLookupForCaseSensitivity(m_caseSensitive);
}

void KeywordList::initLookupForCaseSensitivity(Qt::CaseSensitivity caseSensitive)
{
    /**
     * get right vector to sort, if non-empty, we are done
     */
    auto &vectorToSort = (caseSensitive == Qt::CaseSensitive) ? m_keywordsSortedCaseSensitive : m_keywordsSortedCaseInsensitive;
    if (!vectorToSort.empty()) {
        return;
    }

    /**
     * fill vector with refs to keywords
     */
    vectorToSort.reserve(m_keywords.size());
    for (const auto &keyword : qAsConst(m_keywords)) {
        vectorToSort.push_back(&keyword);
    }

    /**
     * sort with right predicate
     */
    std::sort(vectorToSort.begin(), vectorToSort.end(), [caseSensitive] (const QStringRef &a, const QStringRef &b) { return a.compare(b, caseSensitive) < 0; });
}
