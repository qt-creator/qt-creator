/****************************************************************************
**
** Copyright (C) 2016 Konstantin Tokarev <annulen@yandex.ru>
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
