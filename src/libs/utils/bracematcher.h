/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Konstantin Tokarev <annulen@yandex.ru>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef BRACEMATCHER_H
#define BRACEMATCHER_H

#include "utils_global.h"

#include <QChar>
#include <QSet>
#include <QMap>

QT_BEGIN_NAMESPACE
class QString;
class QTextCursor;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT BraceMatcher
{
public:
    void addBraceCharPair(const QChar opening, const QChar closing);
    void addQuoteChar(const QChar quote);
    void addDelimiterChar(const QChar delim);

    bool shouldInsertMatchingText(const QTextCursor &tc) const;
    bool shouldInsertMatchingText(const QChar lookAhead) const;

    QString insertMatchingBrace(const QTextCursor &tc, const QString &text,
                                const QChar la, int *skippedChars) const;

    bool isOpeningBrace(const QChar c) const { return m_braceChars.contains(c); }
    bool isClosingBrace(const QChar c) const { return m_braceChars.values().contains(c); }
    bool isQuote(const QChar c) const { return m_quoteChars.contains(c); }
    bool isDelimiter(const QChar c) const { return m_delimiterChars.contains(c); }
    bool isKnownChar(const QChar c) const;

private:
    QMap<QChar, QChar> m_braceChars;
    QSet<QChar> m_quoteChars;
    QSet<QChar> m_delimiterChars;
};

}  // namespace Utils

#endif // BRACEMATCHER_H
