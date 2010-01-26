/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "icompletioncollector.h"
#include "itexteditable.h"
#include <QtCore/QRegExp>
#include <algorithm>

using namespace TextEditor;

bool ICompletionCollector::compareChar(const QChar &l, const QChar &r)
{
    if (l == QLatin1Char('_'))
        return false;
    else if (r == QLatin1Char('_'))
        return true;
    else
        return l < r;
}

bool ICompletionCollector::lessThan(const QString &l, const QString &r)
{
    return std::lexicographical_compare(l.begin(), l.end(),
                                        r.begin(), r.end(),
                                        compareChar);
}

bool ICompletionCollector::completionItemLessThan(const CompletionItem &i1, const CompletionItem &i2)
{
    // The order is case-insensitive in principle, but case-sensitive when this would otherwise mean equality
    const QString lower1 = i1.text.toLower();
    const QString lower2 = i2.text.toLower();
    if (lower1 == lower2)
        return lessThan(i1.text, i2.text);
    else
        return lessThan(lower1, lower2);
}

QList<CompletionItem> ICompletionCollector::getCompletions()
{
    QList<CompletionItem> completionItems;

    completions(&completionItems);

    qStableSort(completionItems.begin(), completionItems.end(), completionItemLessThan);

    // Remove duplicates
    QString lastKey;
    QList<CompletionItem> uniquelist;

    foreach (const CompletionItem &item, completionItems) {
        if (item.text != lastKey) {
            uniquelist.append(item);
            lastKey = item.text;
        } else {
            uniquelist.last().duplicateCount++;
        }
    }

    return uniquelist;
}

bool ICompletionCollector::partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems)
{
    // Compute common prefix
    QString firstKey = completionItems.first().text;
    QString lastKey = completionItems.last().text;
    const int length = qMin(firstKey.length(), lastKey.length());
    firstKey.truncate(length);
    lastKey.truncate(length);

    while (firstKey != lastKey) {
        firstKey.chop(1);
        lastKey.chop(1);
    }

    if (ITextEditable *ed = editor()) {
        const int typedLength = ed->position() - startPosition();
        if (!firstKey.isEmpty() && firstKey.length() > typedLength) {
            ed->setCurPos(startPosition());
            ed->replace(typedLength, firstKey);
        }
    }

    return false;
}

void ICompletionCollector::filter(const QList<TextEditor::CompletionItem> &items,
                                  QList<TextEditor::CompletionItem> *filteredItems,
                                  const QString &key,
                                  ICompletionCollector::CaseSensitivity caseSensitivity)
{
    /*
     * This code builds a regular expression in order to more intelligently match
     * camel-case style. This means upper-case characters will be rewritten as follows:
     *
     *   A => [a-z0-9_]*A (for any but the first capital letter)
     *
     * Meaning it allows any sequence of lower-case characters to preceed an
     * upper-case character. So for example gAC matches getActionController.
     *
     * It also implements the first-letter-only case sensitivity.
     */
    QString keyRegExp;
    keyRegExp += QLatin1Char('^');
    bool first = true;
    const QLatin1String wordContinuation("[a-z0-9_]*");
    foreach (const QChar &c, key) {
        if (caseSensitivity == CaseInsensitive ||
            (caseSensitivity == FirstLetterCaseSensitive && !first)) {

            keyRegExp += QLatin1String("(?:");
            if (c.isUpper() && !first)
                keyRegExp += wordContinuation;
            keyRegExp += QRegExp::escape(c.toUpper());
            keyRegExp += "|";
            keyRegExp += QRegExp::escape(c.toLower());
            keyRegExp += QLatin1Char(')');
        } else {
            if (c.isUpper() && !first)
                keyRegExp += wordContinuation;
            keyRegExp += QRegExp::escape(c);
        }

        first = false;
    }
    const QRegExp regExp(keyRegExp);

    const bool hasKey = !key.isEmpty();
    foreach (TextEditor::CompletionItem item, items) {
        if (regExp.indexIn(item.text) == 0) {
            if (hasKey) {
                if (item.text.startsWith(key, Qt::CaseSensitive)) {
                    item.relevance = 2;
                } else if (caseSensitivity != CaseSensitive
                           && item.text.startsWith(key, Qt::CaseInsensitive)) {
                    item.relevance = 1;
                }
            }
            filteredItems->append(item);
        }
    }
}
