// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "keyword.h"
#include "todoitem.h"

namespace Todo {
namespace Internal {

class LineParser
{
public:
    explicit LineParser(const KeywordList &keywordList);

    void setKeywordList(const KeywordList &keywordList);
    QList<TodoItem> parse(const QString &line);

    // This can also be used from KeywordDialog to avoid code duplication
    static bool isKeywordSeparator(const QChar &ch);

private:

    // map key here is keyword start position in the text line
    // and map value is keyword index in m_keywords
    using KeywordEntryCandidates = QMap<int, int> ;

    struct KeywordEntry {
        int keywordIndex;
        int keywordStart;
        QString text;
    };

    KeywordEntryCandidates findKeywordEntryCandidates(const QString &line);
    bool isKeywordAt(int index, const QString &line, const QString &keyword);
    bool isFirstCharOfTheWord(int index, const QString &line);
    bool isLastCharOfTheWord(int index, const QString &line);
    QList<KeywordEntry> keywordEntriesFromCandidates(const QMap<int, int> &candidates, const QString &line);
    QString trimSeparators(const QString &string);
    bool startsWithSeparator(const QString &string);
    bool endsWithSeparator(const QString &string);
    QList<TodoItem> todoItemsFromKeywordEntries(const QList<KeywordEntry> &entries);

    KeywordList m_keywords;
};

} // namespace Internal
} // namespace Todo
