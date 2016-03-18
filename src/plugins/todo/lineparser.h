/****************************************************************************
**
** Copyright (C) 2016 Dmitry Savchenko
** Copyright (C) 2016 Vasiliy Sorokin
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    typedef QMap<int, int> KeywordEntryCandidates;

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
