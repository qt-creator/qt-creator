/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtGlobal>

#include <cplusplus/Token.h>
#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QTextCursor)
QT_FORWARD_DECLARE_CLASS(QTextBlock)
QT_FORWARD_DECLARE_CLASS(QChar)

namespace CPlusPlus {

class CPLUSPLUS_EXPORT MatchingText
{
public:
    using IsNextBlockDeeperIndented = std::function<bool(const QTextBlock &textBlock)>;
    static bool contextAllowsAutoParentheses(const QTextCursor &cursor,
                                             const QString &textToInsert,
                                             IsNextBlockDeeperIndented isNextIndented
                                                = IsNextBlockDeeperIndented());
    static bool contextAllowsAutoQuotes(const QTextCursor &cursor,
                                        const QString &textToInsert);
    static bool contextAllowsElectricCharacters(const QTextCursor &cursor);

    static bool shouldInsertMatchingText(const QTextCursor &tc);
    static bool shouldInsertMatchingText(QChar lookAhead);

    static bool isInCommentHelper(const QTextCursor &currsor, Token *retToken = 0);
    static CPlusPlus::Kind stringKindAtCursor(const QTextCursor &cursor);

    static QString insertMatchingBrace(const QTextCursor &tc, const QString &text,
                                       QChar lookAhead, bool skipChars, int *skippedChars);
    static QString insertMatchingQuote(const QTextCursor &tc, const QString &text,
                                       QChar lookAhead, bool skipChars, int *skippedChars);
    static QString insertParagraphSeparator(const QTextCursor &tc);
};

} // namespace CPlusPlus
