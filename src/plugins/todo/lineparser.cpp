/**************************************************************************
**
** Copyright (c) 2013 Dmitry Savchenko
** Copyright (c) 2013 Vasiliy Sorokin
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    return ch.isSpace()
        || (ch == QLatin1Char(':'))
        || (ch == QLatin1Char('/'))
        || (ch == QLatin1Char('*'))
        || (ch == QLatin1Char('('));
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
        item.iconResource = m_keywords.at(entry.keywordIndex).iconResource;
        todoItems << item;
    }

    return todoItems;
}

} // namespace Internal
} // namespace Todo
