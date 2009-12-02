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

