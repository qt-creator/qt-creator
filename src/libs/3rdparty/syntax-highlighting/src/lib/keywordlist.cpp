/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "definition_p.h"
#include "keywordlist_p.h"
#include "ksyntaxhighlighting_logging.h"
#include "repository.h"

#include <QXmlStreamReader>

#include <algorithm>

using namespace KSyntaxHighlighting;

bool KeywordList::contains(QStringView str, Qt::CaseSensitivity caseSensitive) const
{
    /**
     * get right vector to search in
     */
    const auto &vectorToSearch = (caseSensitive == Qt::CaseSensitive) ? m_keywordsSortedCaseSensitive : m_keywordsSortedCaseInsensitive;

    /**
     * search with right predicate
     */
    return std::binary_search(vectorToSearch.begin(), vectorToSearch.end(), QStringView(str), [caseSensitive](QStringView a, QStringView b) {
        return a.compare(b, caseSensitive) < 0;
    });
}

void KeywordList::load(QXmlStreamReader &reader)
{
    Q_ASSERT(reader.name() == QLatin1String("list"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    m_name = reader.attributes().value(QLatin1String("name")).toString();

    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String("item")) {
                m_keywords.append(reader.readElementText().trimmed());
                reader.readNextStartElement();
                break;
            } else if (reader.name() == QLatin1String("include")) {
                m_includes.append(reader.readElementText().trimmed());
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
        vectorToSort.push_back(keyword);
    }

    /**
     * sort with right predicate
     */
    std::sort(vectorToSort.begin(), vectorToSort.end(), [caseSensitive](QStringView a, QStringView b) {
        return a.compare(b, caseSensitive) < 0;
    });
}

void KeywordList::resolveIncludeKeywords(DefinitionData &def)
{
    while (!m_includes.isEmpty()) {
        const auto kw_include = std::move(m_includes.back());
        m_includes.pop_back();

        const auto idx = kw_include.indexOf(QLatin1String("##"));
        KeywordList *keywords = nullptr;

        if (idx >= 0) {
            auto listName = kw_include.left(idx);
            auto defName = kw_include.mid(idx + 2);
            auto includeDef = def.repo->definitionForName(defName);
            if (includeDef.isValid()) {
                auto defData = DefinitionData::get(includeDef);
                defData->load(DefinitionData::OnlyKeywords(true));
                keywords = defData->keywordList(listName);
            } else {
                qCWarning(Log) << "Unable to resolve external include keyword for definition" << defName << "in" << def.name;
            }
        } else {
            keywords = def.keywordList(kw_include);
        }

        if (keywords) {
            if (this != keywords) {
                keywords->resolveIncludeKeywords(def);
            }
            m_keywords += keywords->m_keywords;
        } else {
            qCWarning(Log) << "Unresolved include keyword" << kw_include << "in" << def.name;
        }
    }
}
