/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Dmitry Savchenko.
** Copyright (c) 2010 Vasiliy Sorokin.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "lineparser.h"

#include <QMap>

namespace Todo {
namespace Internal {

LineParser::LineParser()
{
}

LineParser::LineParser(const KeywordList &keywordList)
{
    setKeywordList(keywordList);
}

void LineParser::setKeywordList(const KeywordList &keywordList)
{
    m_keywords = keywordList;
}

QList<TodoItem> LineParser::parse(const QString &line)
{
    QMap<int, int> entryCandidates = findKeywordEntryCandidates(line);
    QList<KeywordEntry> entries = keywordEntriesFromCandidates(entryCandidates, line);
    return todoItemsFromKeywordEntries(entries);
}

LineParser::KeywordEntryCandidates LineParser::findKeywordEntryCandidates(const QString &line)
{
    KeywordEntryCandidates entryCandidates;

    for (int i = 0; i < m_keywords.count(); ++i) {
        int searchFrom = -1;
        forever {
            const int index = line.lastIndexOf(m_keywords.at(i).name
                                               + QLatin1Char(':'), searchFrom);

            if (index == -1)
                break; // 'forever' loop exit condition

            searchFrom = index - line.length() - 1;

            if (!isFirstCharOfTheWord(index, line))
                continue;

            entryCandidates.insert(index, i);
        }
    }

    return entryCandidates;
}

bool LineParser::isFirstCharOfTheWord(int index, const QString &line)
{
    return (index == 0) || !line.at(index - 1).isLetterOrNumber();
}

QList<LineParser::KeywordEntry> LineParser::keywordEntriesFromCandidates(
    const QMap<int, int> &candidates, const QString &line)
{
    // Ensure something is found
    if (candidates.isEmpty())
        return QList<KeywordEntry>();

    // Convert candidates to entries
    QList<KeywordEntry> entries;
    QMapIterator<int, int> i(candidates);
    i.toBack();

    while (i.hasPrevious()) {
        i.previous();

        KeywordEntry entry;

        entry.keywordStart = i.key();
        entry.keywordIndex = i.value();

        int keywordLength = m_keywords.at(entry.keywordIndex).name.length() + 1; // include colon

        int entryTextLength = -1;
        if (!entries.empty())
            entryTextLength = entries.last().keywordStart - (entry.keywordStart + keywordLength);

        entry.text = line.mid(entry.keywordStart + keywordLength, entryTextLength).trimmed();

        if (entry.text.isEmpty() && !entries.empty())
            // Take the text form the previous entry, consider:
            // '<keyword1>: <keyword2>: <some text>'
            entry.text = entries.last().text;

        entries << entry;
    }

    return entries;
}

QList<TodoItem> LineParser::todoItemsFromKeywordEntries(const QList<KeywordEntry> &entries)
{
    QList<TodoItem> todoItems;

    foreach (const KeywordEntry &entry, entries) {
        TodoItem item;
        item.text =  m_keywords.at(entry.keywordIndex).name
                     + QLatin1String(": ") + entry.text;
        item.color = m_keywords.at(entry.keywordIndex).color;
        item.iconResource = m_keywords.at(entry.keywordIndex).iconResource;
        todoItems << item;
    }

    return todoItems;
}

} // namespace Internal
} // namespace Todo
