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

#include "lineparser.h"

#include <QMap>

namespace Todo {
namespace Internal {

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

bool LineParser::isKeywordSeparator(const QChar &ch)
{
    return ch.isSpace() || (ch == ':') || (ch == '/') || (ch == '*') || (ch == '(');
}

LineParser::KeywordEntryCandidates LineParser::findKeywordEntryCandidates(const QString &line)
{
    KeywordEntryCandidates entryCandidates;

    for (int i = 0; i < m_keywords.count(); ++i) {
        int searchFrom = -1;
        forever {
            const int index = line.lastIndexOf(m_keywords.at(i).name, searchFrom);

            if (index == -1)
                break; // 'forever' loop exit condition

            searchFrom = index - line.length() - 1;

            if (isKeywordAt(index, line, m_keywords.at(i).name))
                entryCandidates.insert(index, i);
        }
    }

    return entryCandidates;
}

bool LineParser::isKeywordAt(int index, const QString &line, const QString &keyword)
{
    if (!isFirstCharOfTheWord(index, line))
        return false;

    if (!isLastCharOfTheWord(index + keyword.length() - 1, line))
        return false;

    return true;
}

bool LineParser::isFirstCharOfTheWord(int index, const QString &line)
{
    return (index == 0) || isKeywordSeparator(line.at(index - 1));
}

bool LineParser::isLastCharOfTheWord(int index, const QString &line)
{
    return (index == line.length() - 1) || isKeywordSeparator(line.at(index + 1));
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

        int keywordLength = m_keywords.at(entry.keywordIndex).name.length();

        int entryTextLength = -1;
        if (!entries.empty())
            entryTextLength = entries.last().keywordStart - (entry.keywordStart + keywordLength);

        entry.text = line.mid(entry.keywordStart + keywordLength, entryTextLength);

        if (trimSeparators(entry.text).isEmpty() && !entries.empty())
            // Take the text form the previous entry, consider:
            // '<keyword1>: <keyword2>: <some text>'
            entry.text = entries.last().text;

        entries << entry;
    }

    return entries;
}

QString LineParser::trimSeparators(const QString &string)
{
    QString result = string.trimmed();

    while (startsWithSeparator(result))
        result = result.right(result.length() - 1);

    while (endsWithSeparator(result))
        result = result.left(result.length() - 1);

    return result;
}

bool LineParser::startsWithSeparator(const QString &string)
{
    return !string.isEmpty() && isKeywordSeparator(string.at(0));
}

bool LineParser::endsWithSeparator(const QString &string)
{
    return !string.isEmpty() && isKeywordSeparator(string.at(string.length() - 1));
}

QList<TodoItem> LineParser::todoItemsFromKeywordEntries(const QList<KeywordEntry> &entries)
{
    QList<TodoItem> todoItems;

    foreach (const KeywordEntry &entry, entries) {
        TodoItem item;
        item.text =  m_keywords.at(entry.keywordIndex).name + entry.text;
        item.color = m_keywords.at(entry.keywordIndex).color;
        item.iconType = m_keywords.at(entry.keywordIndex).iconType;
        todoItems << item;
    }

    return todoItems;
}

} // namespace Internal
} // namespace Todo
