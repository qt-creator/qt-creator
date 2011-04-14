/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "icompletioncollector.h"

#include "completionsettings.h"
#include "itexteditor.h"

#include <QtCore/QRegExp>
#include <algorithm>

using namespace TextEditor;
using namespace TextEditor::Internal;

namespace TextEditor {
namespace Internal {

class ICompletionCollectorPrivate
{
public:
    CompletionSettings m_completionSettings;
};

} // namespace Internal
} // namespace TextEditor

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

ICompletionCollector::ICompletionCollector(QObject *parent)
    : QObject(parent)
    , m_d(new Internal::ICompletionCollectorPrivate)
{
}

ICompletionCollector::~ICompletionCollector()
{
    delete m_d;
}

void ICompletionCollector::setCompletionSettings(const CompletionSettings &settings)
{
    m_d->m_completionSettings = settings;
}

const CompletionSettings &ICompletionCollector::completionSettings() const
{
    return m_d->m_completionSettings;
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

bool ICompletionCollector::partiallyComplete(const QList<TextEditor::CompletionItem> &items)
{
    if (! m_d->m_completionSettings.m_partiallyComplete)
        return false;
    if (items.size() >= 100)
        return false;

    QList<TextEditor::CompletionItem> completionItems = items;
    sortCompletion(completionItems);

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

    if (ITextEditor *ed = editor()) {
        const int typedLength = ed->position() - startPosition();
        if (!firstKey.isEmpty() && firstKey.length() > typedLength) {
            ed->setCursorPosition(startPosition());
            ed->replace(typedLength, firstKey);
        }
    }

    return false;
}

void ICompletionCollector::sortCompletion(QList<TextEditor::CompletionItem> &completionItems)
{
    qStableSort(completionItems.begin(), completionItems.end(),
        &ICompletionCollector::completionItemLessThan);
}

void ICompletionCollector::filter(const QList<TextEditor::CompletionItem> &items,
                                  QList<TextEditor::CompletionItem> *filteredItems,
                                  const QString &key)
{
    const TextEditor::CaseSensitivity caseSensitivity = m_d->m_completionSettings.m_caseSensitivity;

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
        if (caseSensitivity == TextEditor::CaseInsensitive ||
            (caseSensitivity == TextEditor::FirstLetterCaseSensitive && !first)) {

            keyRegExp += QLatin1String("(?:");
            if (c.isUpper() && !first)
                keyRegExp += wordContinuation;
            keyRegExp += QRegExp::escape(c.toUpper());
            keyRegExp += QLatin1Char('|');
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

    foreach (const TextEditor::CompletionItem &item, items)
        if (regExp.indexIn(item.text) == 0)
            filteredItems->append(item);
}

bool ICompletionCollector::shouldRestartCompletion()
{
    return true;
}
