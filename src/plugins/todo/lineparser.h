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

#ifndef LINEPARSER_H
#define LINEPARSER_H

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

#endif // LINEPARSER_H
