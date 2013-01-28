/**************************************************************************
**
** Copyright (c) 2013 Konstantin Tokarev <annulen@yandex.ru>
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
